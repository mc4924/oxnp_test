#ifndef __SWMR_RINGBUFFER
#define __SWMR_RINGBUFFER

#include <cstddef>
#include <string>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>


// The data structure that is in shared memory
template <typename T,size_t BUF_SIZE,size_t NUM_READERS> class swmr_ringbuffer_base;


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
 * The ring buffer can have up to NUM_READERS readers. Each is identified by an
 * unsigned integer 0, 1, ... (NUM_READERS-1).
 *
 * This ringbuffer is to be used for communication between different therads/processes,
 * so all methods are thread/process safe (using BOOST interprocess::named_mutex).
 * To ensure inter-process capability the ring buffer itself is placed in shared memory,
 * using BOOST 'interprocess::managed_shared_memory'.
 *
 * The type of data in the buffer, the size (statically allocated at creation) and
 * the max number of readers are all template parameters.
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
     */
    size_t read_available(
                          size_t reader_id ///< Identifier of the reader >.
                         );

    /**
     * Constructs a ring buffer with name 'name' in a shared memory segment
     */
    swmr_ringbuffer(
                    std::string name,
                    boost::interprocess::managed_shared_memory& segment
                   );

    ~swmr_ringbuffer();
private:
    swmr_ringbuffer() {}; // Cannot construct without arguments

    std::string name;       // name of the ringbuffer in shared memory
    std::string mutex_name; // name of the mutex

    boost::interprocess::named_mutex *mutex=NULL;   // The mutex that make it thread/process safe

    swmr_ringbuffer_base<T,BUF_SIZE,NUM_READERS> *buf=NULL; // Pointer of the ringbuffer data structure in shared memory
};


#include "swmr_ringbuffer.cpp"

#endif