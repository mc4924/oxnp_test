#include <unistd.h>
#include <string>
#include <iostream>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "H5Cpp.h"

#include "cmn.h"

#include "period_repeat.h"


using namespace std;
using namespace boost;
using namespace H5;

// Transformer for the generator/transformer/reader example.
// This process reads data points from the ring buffer named 'ringbuf_in_name',
// transforms them and writes in the ring buffer 'ringbuf_out_name'
// To evenly spread the CPU load, this wakes up every INTERVAL_MSEC
// milliseconds and read the appropriate amount of data points to read POINTS_PER_SEC
// points every seconds.


/// How frequently to wake up to read the points
static const unsigned int INTERVAL_MSEC=40;

/// How many points max to read every time
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;


//------------- Modified with the command line arguments --------
unsigned int seconds_to_run=600;
unsigned int reader_id=0;
string ringbuf_in_name,ringbuf_out_name;

void parse_args(int argc,char *argv[]);


int main(int argc, char *argv[])
{
    parse_args(argc,argv);

    // Setup for the ring buffers, which must have been ALREDAY CREATED (this
    // just opens them)
    interprocess::managed_shared_memory segment(interprocess::open_only, SHARED_MEM_NAME);
    shm_ringbuf  buf_in(ringbuf_in_name.c_str(),segment);
    shm_ringbuf  buf_out(ringbuf_out_name.c_str(),segment);


    size_t j=0;   // counter for read/written points

    period_repeat(

        INTERVAL_MSEC, // How frequently to repeat

        // What to do each time
        [&] {
            data_point_t membuf[POINTS_PER_INTERVAL];

            // Read the data from one ring buffer, one chunk at a time
            unsigned int n=buf_in.read(reader_id,membuf,POINTS_PER_INTERVAL);

            for (int k=0;k<n;k++)
                membuf[k] = transform(j++,membuf[k]);

            // Write to the other buffer
            buf_out.write(membuf,n);
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
        ("inbuf", po::value<string>() ,"name of the input ringbuffer")
        ("id", po::value<unsigned int>(), "reader Id for the input ring buffer")
        ("outbuf", po::value<string>() ,"name of the output ringbuffer")
        ("seconds", po::value<unsigned int>(), "how many seconds to run")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        exit(0);
    }

    if (vm.count("inbuf"))
        ringbuf_in_name= vm["inbuf"].as<string>();
    if (ringbuf_in_name.empty()) {
        cerr << "ERROR: need to specify input ringbuffer name" << endl;
        exit(-1);
    }

    if (vm.count("outbuf"))
        ringbuf_out_name= vm["outbuf"].as<string>();
    if (ringbuf_out_name.empty()) {
        cerr << "ERROR: need to specify output ringbuffer name" << endl;
        exit(-1);
    }

    if (vm.count("id"))
        reader_id= vm["id"].as<unsigned int>();

    if (vm.count("seconds"))
        seconds_to_run= vm["seconds"].as<unsigned int>();
}