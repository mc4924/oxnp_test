#include <cstddef>

/**
 * Data structure that implements the guts of the ring buffer.
 * This is placed in shared memory.
 * The locking (mutual exclusion) is not applied in this data structure,
 * but in the swmr_ringbuffer thet wraps it (cannot keep the mutex
 * reference/pointer in shared memory)
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS> class swmr_ringbuffer_base {
public:

    /*
     * This is the hard bit. Must verify all the read indices to see if we
     * are overrunning one or more readers.
     */
    size_t write(const T* data, size_t n) {
        // Can never write more than BUF_SIZE elements;
        if (n>BUF_SIZE)
            n=BUF_SIZE;

        // Depending of the position of the write index and the value of 'n', writing
        // 'n' elements migth require a wrap around at the buffer bottom.
        // So we might have to do two writes into the buffer: 'n1' and 'n2' are the number
        // of elements for the two writes.
        size_t n1=BUF_SIZE-write_index;
        size_t n2=0;
        if (n>n1)
            n2=(n-n1);
        else
            n1=n;

        // Copy the data in the buffer, in one or two moves, as required
        memcpy(buf+write_index,data,n1*sizeof(T));
        if (n2)
            memcpy(buf,data+n1,n2*sizeof(T));

        // The new write index will be this one
        size_t new_write_index = (write_index+n) % BUF_SIZE;


        // For each of the readers, if it has been too slow to read,
        // we move forward its read pointer (data is lost for that reader).
        for (auto &rd_desc : reader_descr) {

            // If we have overrun the read index, update it to skip all the 'lost'
            // (overwritten) data.
            // This depends on the position of the write inded BEFORE and AFTER
            // this write (write_index, new_write_index), if we had a wrap around (n2>0)
            // and of course the position of the read index.
            // Note that (write_index==rd_desc.index) can mean 'buffer full' or 'buffer
            // empty' depending if 'available' is 0 or not.
            if (
                 ( (write_index>rd_desc.index)  && (n2>0) && (new_write_index>rd_desc.index) ) ||
                 ( (write_index<rd_desc.index)  && (new_write_index>rd_desc.index) ) ||
                 ( (write_index==rd_desc.index) && (rd_desc.available>0) )
               ) {
                rd_desc.index= new_write_index;
            }

            // Increment the 'available' counter, clipping at BUF_SIZE
            rd_desc.available += n;
            if (rd_desc.available>BUF_SIZE)
                rd_desc.available=BUF_SIZE;
        }

        write_index = new_write_index;
        return n;
    }

    /*
     * Reading is easier because we cannot overrun.
     * we need just to keep track of the wrap around.
     */
    size_t read(size_t reader,T* read_buf,size_t n) {
        if (reader>=NUM_READERS)
            return 0;

        auto &rd=reader_descr[reader]; // Just for shorthand

        // cannot read more than what's available
        if (n>rd.available)
            n=rd.available;

        // Depending of the position of the read index and the value of 'n', reading
        // 'n' elements migth require a wrap around at the buffer bottom.
        // So we might have to do two read from the buffer: 'n1' and 'n2' are the number
        // of elements for the two reads.
        size_t n1=BUF_SIZE-rd.index; // how many to read to the end of the buffer
        size_t n2=0;                 // if we wrap around when reading will be non zero
        if (n>n1)
            n2=n-n1; // wrap around when reading: how many to read from start of buf
        else
            n1=n;

        // Copy the data from the buffer in one or two moves, as required
        memcpy(read_buf,buf+rd.index,n1*sizeof(T));
        if (n2)
            memcpy(read_buf+n1,buf,n2*sizeof(T));

        // Update read index and element counter
        rd.index = (rd.index+n) % BUF_SIZE;
        rd.available -= n;

        return n;
    }


    size_t read_available(size_t reader) {
        if (reader>=NUM_READERS)
            return 0;
        return reader_descr[reader].available;
    }

    void init() {
        write_index=0;
        for (auto& desc : reader_descr) {desc.index=0;desc.available=0;}
    }
private:

    // Only one writer, only one write index (pointer), i.e. where the next
    // element will be written
    size_t write_index;

    // One entry for each reader: the index of where next element will be read
    // and how many are available to read
    struct {
        size_t index;        // The index (pointer) to the element that would be read next
        uint32_t available;  // How many elements are available to read. Make it 32 bit to ensure reading is atomic
    } reader_descr [NUM_READERS];

    // Where the data is stored
    T buf[BUF_SIZE];
};


/*
 * Wraps the write with the mutex
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
size_t swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::write(const T* data, size_t n) {
    mutex->lock();
    size_t ret= buf->write(data,n);
    mutex->unlock();
    return ret;
}

/*
 * No need to wrap the write with the mutex, as it is already done inside
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
void swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::write(const T& data) {
    write(&data,1);
}

/*
 * Wraps the read with the mutex
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
size_t swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::read(size_t reader,T* read_buf,size_t n) {
    mutex->lock();
    size_t ret= buf->read(reader,read_buf,n);
    mutex->unlock();
    return ret;
}

/*
 * No need to wrap the write with the mutex, as reading teh counter is atomic
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
size_t swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::read_available(size_t reader) {
    return buf->read_available(reader);
}

/*
 * If it does not find the name ringbuffer in the segment, creates it.
 *
 * FIXIT: this is not thread/process safe
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::swmr_ringbuffer(std::string name,boost::interprocess::managed_shared_memory& segment) : name(name) {
    mutex_name=name+"_mutex";
    buf=segment.find<swmr_ringbuffer_base<T,BUF_SIZE,NUM_READERS>>( name.c_str() ).first;
    if (buf==NULL) {
        // Destroy structures in shared memory, if leftover from previous instance that crashed
        boost::interprocess::named_mutex::remove(mutex_name.c_str());

        buf=segment.construct<swmr_ringbuffer_base<T,BUF_SIZE,NUM_READERS>>( name.c_str() )();
        mutex=new boost::interprocess::named_mutex(boost::interprocess::create_only,mutex_name.c_str());
        buf->init();
    } else {
        mutex=new boost::interprocess::named_mutex(boost::interprocess::open_only,mutex_name.c_str());
    }
}

/*
 * FIXIT: should we remove the ringbuffer from the segment?
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::~swmr_ringbuffer() {
    boost::interprocess::named_mutex::remove(mutex_name.c_str());
}
