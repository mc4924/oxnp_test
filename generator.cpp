#include <unistd.h>
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>

#include "cmn.h"

using namespace std;
using namespace boost;

// Generator for the generator/reader example.
// This is the code for the generator process which generates a continuous
// sequence of data points and puts them in a ring buffer.
// To evenly spread the CPU load, the generator wakes up every INTERVAL_MSEC
// milliseconds and produces the appropriate amount of data points



// Size of the buffer in bytes
static const unsigned int BUF_SIZE_BYTES=BUF_SIZE*sizeof(data_point_t);


static const unsigned int INTERVAL_MSEC=20;
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;

static const unsigned int SHM_ADDTIONAL=2048;



struct shm_remove {
   shm_remove() { remove_all(); }
   ~shm_remove(){ remove_all(); }
   void remove_all() {
       interprocess::shared_memory_object::remove(SHARED_MEM_NAME);
   }
} remover;

void signal_handler(const system::error_code& error, int signal_number)
{
    exit(0);
}

int main()
{

    try {
        // Handling of SIGINT
        asio::io_service     signal_ios;

        // Construct a signal set registered for process termination.
        boost::asio::signal_set signals(signal_ios, SIGINT);

        // Start a background asynchronous wait for SIGINT to occur.
        signals.async_wait(signal_handler);
        thread(bind(&asio::io_service::run,boost::ref(signal_ios))).detach();

        unsigned long count=0;

        asio::io_service     ios;
        posix_time::milliseconds interval(INTERVAL_MSEC);

        interprocess::managed_shared_memory segment(interprocess::create_only, SHARED_MEM_NAME, BUF_SIZE_BYTES+SHM_ADDTIONAL);

        shm_ringbuf buf(RINGBUF_NAME,segment);

        // Construct a timer with an absolute expiry time.
        posix_time::ptime  next_t=posix_time::microsec_clock::local_time()+interval;
        asio::deadline_timer timer(ios,next_t);
        timer.wait();
        ios.run();

        double t=0;
        while (1) {
            for (int k=0;k<POINTS_PER_INTERVAL;k++) {

                // Write one data point
                size_t n=buf.write(2.78*sin(t)+3.14*cos(t/10));

                if (n!=1)
                    cout << "ERROR: write returns "<< n << endl;
                t+=0.01;
            }
            count++;
            if ((count%50)==0) {
                cout << "buffer contains "<< buf.read_available(0) << " elements " <<  endl;
            }

            next_t += interval;
            timer.expires_at(next_t);
            timer.wait();
        }
    } catch (...) {
        cout << "Got an exception\n";
    }

    return 0;
}