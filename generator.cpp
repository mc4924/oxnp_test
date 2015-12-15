#include <unistd.h>
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include "cmn.h"

using namespace std;
using namespace boost;

// Generator for the generator/reader example.
// This is the code for the generator process which generates a continuous
// sequence of data points and puts them in a ring buffer.
// To evenly spread the CPU load, the generator wakes up every INTERVAL_MSEC
// milliseconds and produces the appropriate amount of data points



static const unsigned int INTERVAL_MSEC=20;
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;

static const unsigned int SHM_ADDTIONAL=512;



struct shm_remove {
   shm_remove() { remove_all(); }
   ~shm_remove(){ remove_all(); }
   void remove_all() {
       interprocess::shared_memory_object::remove(SHARED_MEM_NAME);
       interprocess::named_mutex::remove(MUTEX_NAME);
   }
} remover;


int main()
{
    unsigned long count=0;

    asio::io_service     ios;
    posix_time::milliseconds interval(INTERVAL_MSEC);

    interprocess::managed_shared_memory segment(interprocess::create_only, SHARED_MEM_NAME, sizeof(shm_data_ringbuf)+SHM_ADDTIONAL);
    interprocess::named_mutex mutex(interprocess::create_only,MUTEX_NAME);

    shm_data_ringbuf* buf=segment.construct<shm_data_ringbuf>(RINGBUF_NAME)();
    if (buf==NULL) {
        cerr << "cannot construct " << RINGBUF_NAME << endl;
        exit(-1);
    }

    // Construct a timer with an absolute expiry time.
    posix_time::ptime  next_t=posix_time::microsec_clock::local_time()+interval;
    asio::deadline_timer timer(ios,next_t);
    timer.wait();
    ios.run();

    double t=0;
    while (1) {
        for (int k=0;k<POINTS_PER_INTERVAL;k++) {
            mutex.lock();
            if (buf->write_available()==0)
                buf->pop();
            buf->push(2.78*sin(t)+3.14*cos(t/10));
            mutex.unlock();

            t+=0.01;
        }
        count++;
        if ((count%50)==0) {
            cout << "buffer contains "<< buf->read_available() << " elements " <<  endl;
        }

        next_t += interval;
        timer.expires_at(next_t);
        timer.wait();
    }

    return 0;
}