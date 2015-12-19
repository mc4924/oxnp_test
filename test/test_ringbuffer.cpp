#include <unistd.h>
#include <iostream>
#include "swmr_ringbuffer.h"

using namespace boost::interprocess;
using namespace std;

/*

Code to test the swmr_ringbuffer data structure.

Runs a 'one writer'/'two readers' test. The writer writes a sequence of integers
in the ring buffer (length of sequence bigger than the ring buffer size to verify wrap-around).

The readers read the sequence and verify for the values to be in the correct increasing order.

If the test is succesful, the following messages are printed (in a random order):

  Reader o: all reads completed
  Reader 1: all reads completed

The test code (test/test_ringbuffer.cpp) uses fork() twice to spawn the two readers processes.

*/


static const char* SHM_NAME="THE_TEST_MEMORY_SEGMENT";

static const char* RINGBUF_NAME="THE_TEST_RING_BUFFER";

static const size_t BUF_SIZE=10000;

static const size_t NUM_WRITES=100000; // We write this many points in the buffer

static const size_t MAX_READERS=3;


typedef swmr_ringbuffer<int,BUF_SIZE,MAX_READERS> ringbuf_t; // Just a shorthand


/**
 * Creates the shared memory data structures.
 */
void init()
{
    // Remove first in case left from previous abort
    ringbuf_t::remove(RINGBUF_NAME);
    shared_memory_object::remove(SHM_NAME);

    // Create
    managed_shared_memory* shm_segment=new managed_shared_memory(create_only, SHM_NAME, ringbuf_t::get_shm_size()+400);
    ringbuf_t::construct(RINGBUF_NAME,*shm_segment);
}


/**
 * Just writes a series of incrementing integers.
 */
void writer()
{
    // Open shared memeory buffer
    managed_shared_memory segment(open_only, SHM_NAME);
    ringbuf_t buf(RINGBUF_NAME,segment);

    // First do a 'array' write...
    const size_t N_ARRAY=77;
    int array[N_ARRAY];
    for (int k=0;k<N_ARRAY;k++)
        array[k]=k;

    buf.write(array,N_ARRAY);

    // ... Then a sequence of single element writes
    for (int k=N_ARRAY;k<NUM_WRITES;k++)
        buf.write(k);
}


/**
 * Just reads NUM_WRITES integer from the buffer using 'reader_id'
 * and checks they are in incrementing order, starting at 0.
 * In case of an error, prints a message on stderr and terminates
 * If all values are read succesfully, prints:
 *   Reader X: all reads completed
 */
void reader(size_t reader_id)
{
    // Open shared memeory buffer
    managed_shared_memory segment(open_only, SHM_NAME);
    ringbuf_t buf(RINGBUF_NAME,segment);


    for (int k=0;k<NUM_WRITES;k++) {
        int x;

        // if nothing to read,wait a bit and retry
        while (buf.read(reader_id,&x,1)!=1)
            usleep(10);

        if (x!=k) {
            cerr << "ERROR: Reader " << reader_id << " mismatch: " << x << "read instead of " << k << endl;
            exit(-1);
        }
    }
    cout << "Reader " << reader_id << ": all reads completed" << endl;
}



/**
 * Creates the data structures, spawns two children (the readers),
 * does the writing and cleans up.
 */
int main(int argc,char * argv[])
{
    init();

    switch (fork()) {
        case -1:
            perror("ERROR: fork [1] fails");
            exit(-1);
            break;
        case 0:
            // Child
            switch (fork()) {
                case -1:
                    perror("ERROR: fork [2] fails");
                    exit(-1);
                    break;
                case 0:
                    // grand child
                    reader(0);
                    break;
                default:
                    // the child
                    reader(1);
            }
            break;
        default:
            writer();
    }

    // Wait a bit to make sure the readers have finished before deleting the
    // shared memory structures.
    // Not really correct, should do a proper rendez-vous with the children.
    sleep(3);
    ringbuf_t::remove(RINGBUF_NAME);
    shared_memory_object::remove(SHM_NAME);
}