#include <unistd.h>
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <boost/asio/deadline_timer.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/sync/named_mutex.hpp>
#include <boost/asio/signal_set.hpp>
#include <boost/thread.hpp>
#include "H5Cpp.h"

#include "cmn.h"


using namespace std;
using namespace boost;
using namespace H5;

// Reader for the generator/reader example.
// This process reads data points from the ring buffer named RINGBUF_NAME and writes
// them on consecutive HDF5 files.


/// How frequently to wake up to read the points
static const unsigned int INTERVAL_MSEC=40;

/// How many points to read every time
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;




// Names for the oputput files and the dataset
const char* file_name_prefix="data/testdata";
const H5std_string DATASET_NAME("dset");



volatile bool running;

void signal_handler(const system::error_code& error, int signal_number)
{
    running=false;
}

int main()
{
    // Handling of SIGINT
    asio::io_service     signal_ios;

    // Construct a signal set registered for process termination.
    boost::asio::signal_set signals(signal_ios, SIGINT);

    // Start a background asynchronous wait for SIGINT to occur.
    signals.async_wait(signal_handler);
    thread(bind(&asio::io_service::run,boost::ref(signal_ios))).detach();

    interprocess::managed_shared_memory segment(interprocess::open_only, SHARED_MEM_NAME);
    shm_ringbuf buf(RINGBUF_NAME,segment);


    asio::io_service     ios;
    posix_time::milliseconds interval(INTERVAL_MSEC);


    // Construct a timer with an absolute expiry time.
    posix_time::ptime  next_t=posix_time::microsec_clock::local_time()+interval;
    asio::deadline_timer timer(ios,next_t);
    timer.wait();
    ios.run();


ofstream refFile("data/testdata.txt");

    DataSpace *filespace=NULL;
    DataSpace *memspace=NULL;
    running=true;
    H5File *output_file=NULL;
    DataSet dataset;
    DataSpace dataspace;
    unsigned long num_points=0,file_num=0;
    while (running) {

        if (output_file==NULL) {
            ostringstream out_file_name;
            out_file_name << file_name_prefix << setfill('0') << setw(3) << file_num++ << ".h5";
            string filename=out_file_name.str();
            output_file=new H5File(H5std_string(filename), H5F_ACC_TRUNC);

            cout << "Writing data into " << filename << endl;

            hsize_t dim[1]={POINTS_PER_FILE};   // How many points in the dataset in this file
            dataspace=DataSpace(1, dim);
            dataset = output_file->createDataSet( DATASET_NAME, PredType::IEEE_F64LE, dataspace );
        }


        data_point_t filebuf[POINTS_PER_INTERVAL];
        unsigned int n=buf.read(0,filebuf,POINTS_PER_INTERVAL);
        if (n!=POINTS_PER_INTERVAL)
            cout << "ERROR: read returns " << n << " instead of " << POINTS_PER_INTERVAL << endl;

for (int k=0;k<POINTS_PER_INTERVAL;k++)
    refFile << filebuf[k] << endl;

        hsize_t memdim[1]={POINTS_PER_INTERVAL};
        hsize_t offset[1]={num_points};
        hsize_t count[1]={POINTS_PER_INTERVAL};
        hsize_t stride[1]={1};
        hsize_t block[1]={1};
        DataSpace memspace(1, memdim);
        dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride, block);
        dataset.write(filebuf, PredType::NATIVE_DOUBLE,memspace,dataspace);

        num_points += n;


        if (num_points==POINTS_PER_FILE) {
            dataset.close();
            output_file->close();
            delete output_file;
            output_file=NULL;
            num_points=0;
        }



        next_t += interval;
        timer.expires_at(next_t);
        timer.wait();
    }

    return 0;
}