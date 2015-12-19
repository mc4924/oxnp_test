#ifndef __SWMR_RINGBUFFER
#define __SWMR_RINGBUFFER

#include <cstddef>
#include <string>
#include <boost/interprocess/sync/named_mutex.hpp>



/**
 * Single Writer, Multiple Reader ring buffer.
 *
 * A ring buffer (circular buffer) where one writer can write data and
 * multiple readers can read.
 * This ring buffer maintains a separate read pointer for each reader so that each
 * reader can independently read the complete data sequence written by the writer.
 *
 * When writing, the writer will never block, but will go on writing data regardless
 * of the readers. If one of the readers is too slow to extract data, that reader
 * will lose (the oldest) data.
 *
 * The implementation uses a semaphore for locking, but it can be easily modified to
 * not use locking (in which case the writer overrun woudl behave differently).
 *
 * The ring buffer can have up to NUM_READERS readers. Each is identified by an
 * unsigned integer 0, 1, ... (NUM_READERS-1).
 *
 * This ringbuffer is to be used for communication between different therads/processes,
 * so all methods are thread/process safe (using BOOST interprocess::named_mutex).
 * To ensure inter-process capability the ring buffer itself is placed in shared memory,
 * using BOOST 'interprocess::managed_shared_memory'.
 * This object itself is not in shared memory, but has a pointer to the shared data
 * memory structure.
 *
 * The type of data in the buffer, the size (statically allocated at creation) and
 * the max number of readers are all template parameters.
 *
 * Note that the shared memory data structures need to be created before calling the
 * constructor: see comments for the constructor and the static method construct()
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS> class swmr_ringbuffer {

public:
    /**
     * Writes 'n' elements in the ring buffer.
     * The write always succedes fully, if n<BUF_SIZE. if n is bigger,
     * only BUF_SIZE points will be written
     *
     * @returns the number of points written
     */
    size_t write(
                const T* data,  ///< pointer to 'n' elements to write >.
                size_t n        ///< number of elements to write >.
                );

    /**
     * Writes a single data element. Always succedes.
     */
    void write(
              const T& data ///< single element to write >.
              );

    /**
     * Reads up to 'n' elements from the ring buffer.
     * Will throw 'std::invalid_argument', if reader_id is invalid.
     *
     * @returns the number of points read (could be less than 'n' if
     *          there were fewer available to read).
     */
    size_t read(
                size_t reader_id, ///< Identifier of the reader >.
                T* read_buf,      ///< Pointer  to where the read data will be copied>.
                size_t n          ///< number of elements to read >.
               );


    /**
     * Returns the number of elements that can be read by a reader.
     * Will throw 'std::invalid_argument', if reader_id is invalid.
     *
     * @returns how many data points are available to read from the buffer
     *          (for thsi reader).
     */
    size_t read_available(
                          size_t reader_id ///< Identifier of the reader >.
                         );

    /**
     * Returns the size of the shared memory data structure.
     */
    static size_t get_shm_size();

    /**
     * Build the data structure in shared memory.
     * Needs to be called (possibly by a different process) before calling
     * the constructor.
     * The shared memeory data structures will persist process termination
     * and need ot be destroyed explicitly with:
     *    swmr_ringbuffer<...>::remove(name)
     */
    static bool construct(
                           const char * name,
                           boost::interprocess::managed_shared_memory& segment
                         );

    /**
     * Destroys the shared memeory data structures.
     */
    static bool remove(
                       const char * name
                      );

    /**
     * This creates the instance that will be used by the process that calls it,
     * but the shared data structure must already have been created beforehand
     * in shared memory (with 'name') using the static method:
     *     swmr_ringbuffer<...>::construct(name,segment)
     */
    swmr_ringbuffer(
                    const char * name,
                    boost::interprocess::managed_shared_memory& segment
                   );

private:

    // The data structure in shared memory. Just the poinetr is stored in this object
    struct shm_buf
    {
        // Only one writer, only one write index (pointer), i.e. where the next
        // element will be written
        size_t write_index;

        // One entry for each reader: the index of where next element will be read
        // and how many are available to read
        struct {
            // The index (pointer) to the element that would be read next
            size_t index;
            // How many elements are available to read. Make it 32 bit to ensure reading
            // is atomic on the majority of reasonably modern CPU architectures
            uint32_t available;
        } reader_descr [NUM_READERS];

        // Where the data is stored
        T data[BUF_SIZE];
    } *buf=NULL;

    boost::interprocess::named_mutex *mutex=NULL;   // The mutex that makes it thread/process safe

    swmr_ringbuffer() {}; // Avoid creation without the needed parameters
};


#include "swmr_ringbuffer.cpp"

#endif