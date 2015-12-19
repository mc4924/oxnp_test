#ifndef __CMN_H
#define __CMN_H

#include "swmr_ringbuffer.h"

// Common definitions for the generator/transformer/reader example code


/// How many points are generated per second
static const unsigned int POINTS_PER_SEC=1000000;


/// How many seconds we want to be able to buffer
static const unsigned int BUF_DEPTH_SEC=10;

// Size of the buffer in data points
static const unsigned int BUF_SIZE=BUF_DEPTH_SEC*POINTS_PER_SEC;


// Names to open/create the interprocess communication structures:
// name of the shared memory segment and names for the two ring buffers/mutexes
static const char* SHARED_MEM_NAME="MySharedMemory";
static const char* RINGBUF_NAME1="RING_BUFFER1";
static const char* RINGBUF_NAME2="RING_BUFFER2";


// All HDF5 files will use a dataset with this name
static const char* DATASET_NAME="dset";




/// Type of the data points we are generating and saving on file
typedef double data_point_t;

/**
 * Generates point 'k' in the sequence.
 */
data_point_t generate(size_t k)
{
    double t=0.01*k;
    return 2.78*sin(t)+3.14*cos(t/10);
}


/**
 * Transforms point 'k' in the sequence
 */
data_point_t transform(size_t k,data_point_t x)
{
    return 2*x;
}



// How many readers can read from the same ring buffer
static const unsigned int MAX_READERS=2;


// Just a shorthand for the ring buffer type. Note that size and max
// number of reader is fixed at compile time
typedef swmr_ringbuffer<data_point_t,BUF_SIZE,MAX_READERS> shm_ringbuf;



#endif