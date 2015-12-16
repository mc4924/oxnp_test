#ifndef __CMN_H
#define __CMN_H

#include "swmr_ringbuffer.h"

// Common definitions for the generator/consumer example


/// Type of the data points we are generating and saving on file
typedef double data_point_t;

/// How many points are generated per second
static const unsigned int POINTS_PER_SEC=1000;


/// How many seconds we want to be able to buffer
static const unsigned int BUF_DEPTH_SEC=10;

// Size of the buffer in data points
static const unsigned int BUF_SIZE=BUF_DEPTH_SEC*POINTS_PER_SEC;


/// How many points to write in every HDF5 file
static const unsigned int POINTS_PER_FILE=5000;


// Names to open/create the interprocess communication structures:
// shared memory segment and ring buffer
static const char* SHARED_MEM_NAME="MySharedMemory";
static const char* RINGBUF_NAME="THE_RING_BUFFER";


typedef swmr_ringbuffer<data_point_t,BUF_SIZE,3> shm_ringbuf;
#endif