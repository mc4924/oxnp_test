#include <unistd.h>
#include <sstream>
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "cmn.h"

using namespace std;
using namespace boost;

// Generator for the generator/reader example.
// This generates a continuous sequence of data points and puts them in a ring buffer.
// To evenly spread the CPU load, the generator wakes up every INTERVAL_MSEC
// milliseconds and produces the appropriate amount of data points to do POINTS_PER_SEC
// points every seconds.



// Size of the buffer in bytes
static const unsigned int BUF_SIZE_BYTES=BUF_SIZE*sizeof(data_point_t);


static const unsigned int INTERVAL_MSEC=20;
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;

static const unsigned int SHM_ADDTIONAL=2048;



//------------- Modified with the command line arguments --------
unsigned int seconds_to_run=600;
unsigned int reader_id=0;

void parse_args(int argc,char *argv[]);



int main(int argc,char *argv[])
{
    parse_args(argc,argv);


    // Ring buffer in shared memory. First remove it, if it was left over from a previous aborted run
    interprocess::shared_memory_object::remove(SHARED_MEM_NAME);
    interprocess::managed_shared_memory segment(interprocess::create_only, SHARED_MEM_NAME, BUF_SIZE_BYTES+SHM_ADDTIONAL);
    shm_ringbuf buf(RINGBUF_NAME,segment);


    // Construct a timer with an absolute expiry time so that we can pace
    // without drift
    asio::io_service     ios;
    posix_time::milliseconds interval(INTERVAL_MSEC);
    posix_time::ptime  next_t=posix_time::microsec_clock::local_time()+interval;
    asio::deadline_timer timer(ios,next_t);
    timer.wait();
    ios.run();


    unsigned long count=0;  // How many interval elapsed so far
    double t=0;
    while (true) {
        for (int k=0;k<POINTS_PER_INTERVAL;k++) {

            // Write one data point
            buf.write(2.78*sin(t)+3.14*cos(t/10));

            t+=0.01;
        }
        count++;

        if ((count%50)==0) {
            cout << "buffer contains "<< buf.read_available(0) << " elements " <<  endl;
        }

        next_t += interval;

        // If not done yet, we restart the timer
        if (count < (seconds_to_run*1000/INTERVAL_MSEC)) {
            timer.expires_at(next_t);
            timer.wait();
        } else
            break;
    }

    interprocess::shared_memory_object::remove(SHARED_MEM_NAME);

    return 0;
}


/**
 * Parsing command line arguments, using BOOST.
 * See BOOST documentation for details.
 */
void parse_args(int argc,char *argv[])
{
    namespace po = boost::program_options;

    // Declare the supported options.
    po::options_description desc("Allowed options");

    desc.add_options()
        ("help", "write this help message")
        ("seconds", po::value<unsigned int>(), "how many seconds to run")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        exit(0);
    }

    if (vm.count("seconds"))
        seconds_to_run= vm["seconds"].as<unsigned int>();
}