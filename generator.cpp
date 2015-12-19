#include <unistd.h>
#include <sstream>
#include <iostream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "cmn.h"

#include "period_repeat.h"


using namespace std;
using namespace boost;

// Generator for the generator/reader example.
// This generates a continuous sequence of data points and puts them in a ring buffer.
// To evenly spread the CPU load, the generator wakes up every INTERVAL_MSEC
// milliseconds and produces the appropriate amount of data points to do POINTS_PER_SEC
// points every seconds.



static const unsigned int INTERVAL_MSEC=20;
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;




//------------- Modified using command line arguments --------
unsigned int seconds_to_run=600;
bool quiet=false;

void parse_args(int argc,char *argv[]);



int main(int argc,char *argv[])
{
    parse_args(argc,argv);

    // Setup for the ring buffer, which must have been ALREDAY CREATED (this
    // just opens it)
    interprocess::managed_shared_memory segment(interprocess::open_only, SHARED_MEM_NAME);
    shm_ringbuf buf(RINGBUF_NAME1,segment);


    size_t j=0;   // counter for generated points

    period_repeat(

        INTERVAL_MSEC, // How frequently to repeat

        // What to do each time
        [&] {

            // Generate a chunk of data points in memory
            data_point_t membuf[POINTS_PER_INTERVAL];
            for (auto &x : membuf)
                x=generate(j++);

            // Write the chunk to the ring buffer
            buf.write(membuf,POINTS_PER_INTERVAL);

            // Every second print a message (just for feedback)
            if (!quiet && (j%POINTS_PER_SEC)==0) {
                cout << "buffer contains [ ";
                for (int k=0;k<MAX_READERS;k++)
                    cout << buf.read_available(k) << " ";
                cout << "] points" <<  endl;
            }
        },

        // Continue while this condition is true
        [&j] {
            return j < (seconds_to_run*POINTS_PER_SEC); // j < (Total points to generate) ?
        }
    );

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
        ("quiet", "don't print any message")
        ("seconds", po::value<unsigned int>(), "how many seconds to run")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        exit(0);
    }

   if (vm.count("quiet"))
        quiet=true;;

    if (vm.count("seconds"))
        seconds_to_run= vm["seconds"].as<unsigned int>();
}