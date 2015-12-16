#ifndef __SWMR_RINGBUFFER
#define __SWMR_RINGBUFFER

#include <cstddef>
#include <string>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>


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
 * The ring buffer can have up to NUM_READERS readers. Each is identifed by an
 * unsigned integer 0, 1, ... (NUM_READERS-1).
 *
 * This is to be used for communication between different therads/processes, so
 * all methods are thread/process safe.
 * To ensure inter-process capability the ring buffer itself is placed in shared memory,
 * using BOOST 'interprocess::managed_shared_memory'.
 */
template <typename T,size_t BUF_SIZE,size_t NUM_READERS> class swmr_ringbuffer {

public:
    /**
     * Writes 'n' elements in the ring buffer.
     * The write always succedes fully, if n>BUF_SIZE.
     */
    size_t write(
                const T* data,  ///< pointer to 'n' elements to write >.
                size_t n        ///< number of elements to write >.
                );

    /**
     * Writes a single data element
     */
    size_t write(
                const T& data ///< single element to write >.
                );

    /**
     * Reads up to 'n' elements from the ring buffer.
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
     * Constructs a ring buffer with name 'name' in a shared memeory segment
     */
    swmr_ringbuffer(
                    std::string name,
                    boost::interprocess::managed_shared_memory& segment
                   );

private:
    swmr_ringbuffer() {}; // Cannot construct without arguments

    boost::interprocess::named_mutex *mutex=NULL;   // The lock that make it thread/process safe

    swmr_ringbuffer_base<T,BUF_SIZE,NUM_READERS> *buf=NULL; // Pointer of thhe data structure in shared memory
};


#include "swmr_ringbuffer.cpp"

#endif