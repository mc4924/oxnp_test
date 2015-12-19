#include <unistd.h>
#include <sstream>
#include <iostream>


#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "cmn.h"

using namespace std;
using namespace boost;




// Additional memory (bytes) required as overahead in the shared_memory_object.
static const unsigned int SHM_OVERHEAD=512;



//------------- Modified with the command line arguments --------
bool remove_only=false;

void parse_args(int argc,char *argv[]);



int main(int argc,char *argv[])
{
    const char *buf_names[]={RINGBUF_NAME1, RINGBUF_NAME2};

    parse_args(argc,argv);

    // First, remove all (structures could be left from previous aborted execution)
    interprocess::shared_memory_object::remove(SHARED_MEM_NAME);
    for (auto name : buf_names)
        shm_ringbuf::remove(name);


    if (remove_only==false) {

        // Create the shared memory segment
        size_t n_bytes=2*shm_ringbuf::get_shm_size()+SHM_OVERHEAD;
        interprocess::managed_shared_memory segment(interprocess::create_only, SHARED_MEM_NAME, n_bytes);

        // Create the two ring buffer areas
        for (auto name : buf_names)
            shm_ringbuf::construct(name,segment);
    }

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
        ("remove", "if specified, shared memory structure will be removed")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        exit(0);
    }

    if (vm.count("remove"))
        remove_only= true;
}