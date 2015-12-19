#include <unistd.h>
#include <string>
#include <fstream>
#include <sstream>
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

// Reader for the generator/reader example.
// This process reads data points from the ring buffer named RINGBUF_NAME and writes
// them on consecutive HDF5 files.
// To evenly spread the CPU load, the reader wakes up every INTERVAL_MSEC
// milliseconds and read the appropriate amount of data points to read POINTS_PER_SEC
// points every seconds.
// Each file will have a single dataset


/// How frequently to wake up to read the points
static const unsigned int INTERVAL_MSEC=40;

/// How many points max to read every time
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;



// Names for the output files and the dataset
const char* file_name_dir="data";
const char* file_name_prefix="testdata";


//------------- Modified with the command line arguments --------
string tag;  // a prefix for the file names
unsigned int seconds_to_run=600;
unsigned int reader_id=0;
/// How many points to write in every HDF5 file
unsigned int file_size=2000000;
string ringbuffer_name="";
bool quiet=false;

void parse_args(int argc,char *argv[]);


int main(int argc, char *argv[])
{
    parse_args(argc,argv);

    // Setup for the ring buffer, which must have been ALREDAY CREATED (this
    // just opens it)
    interprocess::managed_shared_memory segment(interprocess::open_only, SHARED_MEM_NAME);
    shm_ringbuf buf(ringbuffer_name.c_str(),segment);


    // Total points to read
    size_t total_points=seconds_to_run*POINTS_PER_SEC;

    DataSpace *filespace=NULL;
    DataSpace *memspace=NULL;
    H5File *output_file=NULL;
    DataSet dataset;
    DataSpace dataspace;
    unsigned long file_num=0; // will increment with each file generated
    size_t points_in_file;    // how many data points in current file
    size_t num_points;        // how many data points already read in current file

    period_repeat(

        INTERVAL_MSEC,  // How frequently to repeat

        // What to do each time
        [&] {
            // Need to open a new file?
            if (output_file==NULL) {
                ostringstream out_file_name;
                out_file_name << file_name_dir << "/" << tag << "-" << file_name_prefix << "-" << setfill('0') << setw(3) << file_num++ << ".h5";
                string filename=out_file_name.str();
                output_file=new H5File(H5std_string(filename), H5F_ACC_TRUNC);

                if (file_size==0) {
                    // File will contain a random number of data points
                    points_in_file = 2000+double(rand())/RAND_MAX*1.2*BUF_SIZE;
                } else {
                    points_in_file =file_size;
                }
                // if we have only fewer points left to generate in total
                if (total_points<points_in_file)
                    points_in_file=total_points;


                num_points=0;

                // Print a message just for feedback
                if (!quiet)
                    cout << "Reader " << reader_id << ": Writing " << points_in_file << " data points into " << filename << endl;


                hsize_t dim[1]={points_in_file};   // How many points in the dataset in this file
                dataspace=DataSpace(1, dim);
                dataset = output_file->createDataSet( H5std_string(DATASET_NAME), PredType::IEEE_F64LE, dataspace );
            }

            // Read from the ring buffer a chunk of up to POINTS_PER_INTERVAL points
            // (fewer if the points left to write in the file are fewer)
            size_t n_to_read= (points_in_file>POINTS_PER_INTERVAL)? POINTS_PER_INTERVAL : points_in_file;

            data_point_t filebuf[POINTS_PER_INTERVAL];
            unsigned int n=buf.read(reader_id,filebuf,n_to_read);

            // Select the dataset hyperslab and write them
            hsize_t memdim[1]={n};
            hsize_t offset[1]={num_points};
            hsize_t count[1]={n};
            hsize_t stride[1]={1};
            hsize_t block[1]={1};
            DataSpace memspace(1, memdim);
            dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride, block);
            dataset.write(filebuf, PredType::NATIVE_DOUBLE,memspace,dataspace);

            num_points    += n;
            points_in_file -= n;
            total_points  -= n;

            // If finished with the file, close it
            if (points_in_file==0) {
                dataset.close();
                output_file->close();
                delete output_file;
                output_file=NULL;
            }
        },

        // Continue while this condition is true
        [&total_points] {
            return total_points>0;
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
        ("tag", po::value<string>() ,"tag for this reader")
        ("buf", po::value<string>() ,"name of the ringbuffer")
        ("id", po::value<unsigned int>(), "reader Id")
        ("seconds", po::value<unsigned int>(), "how many seconds to run")
        ("size", po::value<unsigned int>(), "file size (in data points). '0' means random size")
    ;

    po::variables_map vm;
    po::store(po::parse_command_line(argc,argv, desc), vm);
    po::notify(vm);

    if (vm.count("help")) {
        cout << desc << "\n";
        exit(0);
    }

    if (vm.count("tag"))
        tag= vm["tag"].as<string>();
    if (tag.empty()) {
        cerr << "ERROR: need to specify a tag for thsi reader" << endl;
        exit(-1);
    }

    if (vm.count("buf"))
        ringbuffer_name= vm["buf"].as<string>();
    if (ringbuffer_name.empty()) {
        cerr << "ERROR: need to specify ringbuffer name" << endl;
        exit(-1);
    }

    if (vm.count("id"))
        reader_id= vm["id"].as<unsigned int>();

    if (vm.count("seconds"))
        seconds_to_run= vm["seconds"].as<unsigned int>();

    if (vm.count("size"))
        file_size= vm["size"].as<unsigned int>();
}