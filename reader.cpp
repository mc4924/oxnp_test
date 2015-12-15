#include <unistd.h>
#include <fstream>
#include <iostream>
#include <boost/lockfree/spsc_queue.hpp>
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


/// How frequently to wake up to read the points
static const unsigned int INTERVAL_MSEC=40;

/// How many points to read every time
static const unsigned int POINTS_PER_INTERVAL=(POINTS_PER_SEC/1000)*INTERVAL_MSEC;

// Names for the oputput file and its only dataset
const H5std_string OUT_FILE_NAME("testdata.h5");
const H5std_string DATASET_NAME("dset");


void signal_handler(const system::error_code& error, int signal_number)
{
  if (!error)
  {
    cout << "GOTCHA! (" << signal_number << ")\n";
    exit(0);
  }
}

int main()
{
    unsigned long count=0,num_points=0;

    // Handling of SIGINT
    asio::io_service     signal_ios;

    // Construct a signal set registered for process termination.
    boost::asio::signal_set signals(signal_ios, SIGINT);

    // Start a abckground asynchronous wait for SIGINT to occur.
    signals.async_wait(signal_handler);
    thread(bind(&asio::io_service::run,boost::ref(signal_ios))).detach();

    asio::io_service     ios;
    posix_time::milliseconds interval(INTERVAL_MSEC);

    interprocess::named_mutex mutex(interprocess::open_only,MUTEX_NAME);

    interprocess::managed_shared_memory segment(interprocess::open_only, SHARED_MEM_NAME);
    shm_data_ringbuf* buf=segment.find<shm_data_ringbuf>( RINGBUF_NAME ).first;
    if (buf==NULL) {
        cerr << "cannot find " << RINGBUF_NAME << endl;
        exit(-1);
    }

    H5File file(OUT_FILE_NAME, H5F_ACC_TRUNC);

    hsize_t dims[1]={POINTS_PER_INTERVAL};   // How many points in a dataset
    hsize_t maxdims[1] = {H5S_UNLIMITED};
    hsize_t chunk_dims[1] ={POINTS_PER_INTERVAL};

    DataSpace *dataspace = new DataSpace (1, dims,maxdims);

    // Modify dataset creation property to enable chunking
    DSetCreatPropList prop;
    prop.setChunk(1, chunk_dims);

    // Create the dataset.
    DataSet *dataset = new DataSet(file.createDataSet( DATASET_NAME,PredType::IEEE_F64LE, *dataspace, prop) );
    if (dataset==NULL) {
        cout << "dataset not created\n";
        exit(-1);
    }

    // Construct a timer with an absolute expiry time.
    posix_time::ptime  next_t=posix_time::microsec_clock::local_time()+interval;
    asio::deadline_timer timer(ios,next_t);
    timer.wait();
    ios.run();

    DataSpace *filespace=NULL;
    DataSpace *memspace=NULL;
    for (int k=0;k<10;k++) {
        data_point_t filebuf[POINTS_PER_INTERVAL];
        mutex.lock();
        unsigned int n=buf->pop(filebuf,POINTS_PER_INTERVAL);
        mutex.unlock();

        if (num_points==0) {
            // First write we do in the file
            dataset->write(filebuf, PredType::NATIVE_DOUBLE);
        } else {
            hsize_t offset[1] = {dims[0]};
            hsize_t dimsext[1]= {POINTS_PER_INTERVAL};
            dims[0] += num_points;
            dataset->extend(dims);
            // Select a hyperslab in extended portion of the dataset.
            if (filespace) delete filespace;
            filespace = new DataSpace(dataset->getSpace());
            filespace->selectHyperslab(H5S_SELECT_SET, dimsext, offset);

            // Define memory space.
            if (memspace) delete memspace;
            memspace = new DataSpace(1, dimsext, NULL);

            // Write data to the extended portion of the dataset.
            dataset->write(filebuf, PredType::NATIVE_DOUBLE, *memspace, *filespace);
        }
        num_points += n;

        count++;
        if ((count%50)==0) {
            cout << num_points << " points written" << endl;
        }

        next_t += interval;
        timer.expires_at(next_t);
        timer.wait();
    }
cout << "Almost finished\n";
    prop.close();
    delete filespace;
    delete memspace;
    delete dataspace;
    delete dataset;
cout << "Closing file\n";
    file.close();

    return 0;
}