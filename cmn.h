#ifndef __CMN_H
#define __CMN_H

#include "swmr_ringbuffer.h"

// Common definitions for the generator/consumer example


/// Type of the data points we are generating and saving on file
typedef double data_point_t;

/// How many points are generated per second
static const unsigned int POINTS_PER_SEC=100000;


/// How many seconds we want to be able to buffer
static const unsigned int BUF_DEPTH_SEC=10;

// Size of the buffer in data points
static const unsigned int BUF_SIZE=BUF_DEPTH_SEC*POINTS_PER_SEC;


// Names to open/create the interprocess communication structures:
// shared memory segment and ring buffer
static const char* SHARED_MEM_NAME="MySharedMemory";
static const char* RINGBUF_NAME1="RING_BUFFER1";
static const char* RINGBUF_NAME2="RING_BUFFER2";

// Just a shorthand for the ring buffer type. Note that size and max
// number of reader is fixed at compile time
typedef swmr_ringbuffer<data_point_t,BUF_SIZE,3> shm_ringbuf;
#endif