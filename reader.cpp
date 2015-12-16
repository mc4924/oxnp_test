#include <unistd.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options/parsers.hpp>

#include "H5Cpp.h"

#include "cmn.h"


using namespace std;
using namespace boost;
using namespace H5;

// Reader for the generator/reader example.
// This process reads data points from the ring buffer named RINGBUF_NAME and writes
// them on consecutive HDF5 files.
// To evenly spread the CPU load, the reader wakes up every INTERVAL_MSEC
// milliseconds and read the appropriate amount of data points to read POINTS_PER_SEC
// points every seconds.


/// How frequently to wake up to read the points
static const unsigned int INTERVAL_MSEC=40;

/// How many points max to read every time
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;



// Names for the output files and the dataset
const char* file_name_dir="data";
const char* file_name_prefix="testdata";
const H5std_string DATASET_NAME("dset");


//------------- Modified with the command line arguments --------
unsigned int seconds_to_run=600;
unsigned int reader_id=0;
/// How many points to write in every HDF5 file
unsigned int points_per_file=5000;

void parse_args(int argc,char *argv[]);


int main(int argc, char *argv[])
{

    parse_args(argc,argv);

    interprocess::managed_shared_memory segment(interprocess::open_only, SHARED_MEM_NAME);
    shm_ringbuf buf(RINGBUF_NAME,segment);

    // Construct a timer with an absolute expiry time so that we can pace
    // without drift
    asio::io_service     ios;
    posix_time::milliseconds interval(INTERVAL_MSEC);
    posix_time::ptime  next_t=posix_time::microsec_clock::local_time()+interval;
    asio::deadline_timer timer(ios,next_t);
    timer.wait();
    ios.run();


    DataSpace *filespace=NULL;
    DataSpace *memspace=NULL;
    H5File *output_file=NULL;
    DataSet dataset;
    DataSpace dataspace;
    unsigned long file_num=0,count_intervals=0;
    size_t total_to_read; // how many data points in current file
    size_t num_points;  // how many data points already read in current file
    while (true) {

        // Need to open a new file?
        if (output_file==NULL) {
            ostringstream out_file_name;
            out_file_name << file_name_dir << "/rdr" << reader_id << "-" << file_name_prefix << "-" << setfill('0') << setw(3) << file_num++ << ".h5";
            string filename=out_file_name.str();
            output_file=new H5File(H5std_string(filename), H5F_ACC_TRUNC);

            if (points_per_file==0) {
                // A random number of data points
                total_to_read = 2000+double(rand())/RAND_MAX*1.2*BUF_SIZE;
            } else {
                total_to_read =points_per_file;
            }
            num_points=0;

            cout << "Reader " << reader_id << ": Writing " << total_to_read << " data points into " << filename << endl;


            hsize_t dim[1]={total_to_read};   // How many points in the dataset in this file
            dataspace=DataSpace(1, dim);
            dataset = output_file->createDataSet( DATASET_NAME, PredType::IEEE_F64LE, dataspace );
        }

        size_t n_to_read= (total_to_read>POINTS_PER_INTERVAL)? POINTS_PER_INTERVAL : total_to_read;

        data_point_t filebuf[POINTS_PER_INTERVAL];
        unsigned int n=buf.read(reader_id,filebuf,n_to_read);
        if (n!=n_to_read)
            cout << "ERROR: read returns " << n << " instead of " << n_to_read << endl;

        hsize_t memdim[1]={n_to_read};
        hsize_t offset[1]={num_points};
        hsize_t count[1]={n_to_read};
        hsize_t stride[1]={1};
        hsize_t block[1]={1};
        DataSpace memspace(1, memdim);
        dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride, block);
        dataset.write(filebuf, PredType::NATIVE_DOUBLE,memspace,dataspace);

        num_points    += n;
        total_to_read -= n;


        if (total_to_read==0) {
            dataset.close();
            output_file->close();
            delete output_file;
            output_file=NULL;
        }

        next_t += interval;
        count_intervals++;

        if (count_intervals < (seconds_to_run*1000/INTERVAL_MSEC)) {
            timer.expires_at(next_t);
            timer.wait();
        } else
            break;
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

    if (vm.count("id"))
        reader_id= vm["id"].as<unsigned int>();

    if (vm.count("seconds"))
        seconds_to_run= vm["seconds"].as<unsigned int>();

    if (vm.count("size"))
        points_per_file= vm["size"].as<unsigned int>();
}