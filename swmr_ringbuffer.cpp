#include <cstddef>
#include <stdexcept>
#include <boost/interprocess/sync/scoped_lock.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>


/*
 * This is the hard bit. Must verify all the read indices to see if we
 * are overrunning one or more readers.
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
size_t swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::write(const T* data, size_t n)
{
    // Can never write more than BUF_SIZE elements;
    if (n>BUF_SIZE)
        n=BUF_SIZE;
    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*mutex);

    // Depending of the position of the write index and the value of 'n', writing
    // 'n' elements migth require a wrap around at the buffer bottom.
    // So we might have to do two writes into the buffer: 'n1' and 'n2' are the number
    // of elements for the two writes (n2 is possibly zero)
    size_t n1=BUF_SIZE - buf->write_index;

    size_t n2=0;
    if (n>n1)
        n2=(n-n1);
    else
        n1=n;


    // Copy the data in the buffer, in one or two moves, as required
    memcpy(buf->data+buf->write_index,data,n1*sizeof(T));
    if (n2)
        memcpy(buf->data,data+n1,n2*sizeof(T));

    // The new write index will be this one
    size_t new_write_index = (buf->write_index+n) % BUF_SIZE;


    // For each of the readers, if it has been too slow to read,
    // we move forward its read pointer (data is lost for that reader).
    for (auto &rd_desc : buf->reader_descr) {

        // If we have overrun the read index, update it to skip all the 'lost'
        // (overwritten) data.
        // This depends on the position of the write inded BEFORE and AFTER
        // this write (write_index, new_write_index), if we had a wrap around (n2>0)
        // and of course the position of the read index.
        // Note that (write_index==rd_desc.index) can mean 'buffer full' or 'buffer
        // empty' depending if 'available' is 0 or not.
        if (
             ( (buf->write_index>rd_desc.index)  && (n2>0) && (new_write_index>rd_desc.index) ) ||
             ( (buf->write_index<rd_desc.index)  && (new_write_index>rd_desc.index) ) ||
             ( (buf->write_index==rd_desc.index) && (rd_desc.available>0) )
           ) {
            rd_desc.index= new_write_index;
        }

        // Increment the 'available' counter, clipping at BUF_SIZE
        rd_desc.available += n;
        if (rd_desc.available>BUF_SIZE)
            rd_desc.available=BUF_SIZE;
    }

    buf->write_index = new_write_index;
    return n;
}


/*
 * No need to use the mutex, as it is already done inside
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
void swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::write(const T& data) {
    write(&data,1);
}

/*
 * Reading is easier because we cannot overrun.
 * We need just to keep track of the wrap around at the bottom of the buffer.
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
size_t swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::read(size_t reader,T* read_buf,size_t n)
{
    if (reader>=NUM_READERS)
        throw std::invalid_argument("invalid reader_id in swmr_ringbuffer::read");

    boost::interprocess::scoped_lock<boost::interprocess::named_mutex> lock(*mutex);

    auto &rd=buf->reader_descr[reader]; // Just for shorthand

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
    memcpy(read_buf,buf->data+rd.index,n1*sizeof(T));
    if (n2)
        memcpy(read_buf+n1,buf->data,n2*sizeof(T));

    // Update read index and element counter
    rd.index = (rd.index+n) % BUF_SIZE;
    rd.available -= n;

    return n;
}

/*
 * No need to wrap with the mutex, as reading the counter is atomic
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
size_t swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::read_available(size_t reader) {
    if (reader>=NUM_READERS)
        throw std::invalid_argument("invalid reader_id in swmr_ringbuffer::read_available");
    return buf->reader_descr[reader].available;
}

template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
size_t swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::get_shm_size()
{
    return sizeof *buf;
}

template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
bool swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::construct(
                           const char * name,
                           boost::interprocess::managed_shared_memory& segment
                         )
{
    segment.construct<shm_buf>( name )();
    boost::interprocess::named_mutex mutex(boost::interprocess::create_only,name);
    return true;
}

template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
bool swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::remove(
                       const char * name
                      )
{
    boost::interprocess::named_mutex::remove(name);
    return true;
}


template <typename T,size_t BUF_SIZE,size_t NUM_READERS>
swmr_ringbuffer<T,BUF_SIZE,NUM_READERS>::swmr_ringbuffer(const char * name,boost::interprocess::managed_shared_memory& segment)
{
    buf=segment.find<shm_buf>( name ).first;

    mutex=new boost::interprocess::named_mutex(boost::interprocess::open_only,name);
}
