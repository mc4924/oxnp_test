#include <unistd.h>
#include <string>
#include <vector>
#include <algorithm>
#include <fstream>
#include <sstream>
#include <iostream>

#include "H5Cpp.h"

#include "cmn.h"


using namespace std;
using namespace H5;


bool do_transform=false;
bool quiet=false;


int main(int argc, char *argv[])
{
    // The cmd line arguments are the files to read for verification.
    // put them in a vector
    vector<string> filenames;

    for (int k=1;k<argc;k++) {
       if (strcmp(argv[k],"--transform")==0)
           do_transform=true;
       else
           filenames.push_back(argv[k]);
    }

    // The filenames end with a sequence number, so we sort them to have them
    // all in order
    sort(filenames.begin(),filenames.end());

    size_t mismatch_count=0;


    size_t j=0;
    for (auto filename: filenames) {

        H5File file(H5std_string(filename), H5F_ACC_RDONLY);
        DataSet dataset = file.openDataSet(H5std_string(DATASET_NAME));
        DataSpace dataspace=dataset.getSpace();

        vector<hsize_t> dims(dataspace.getSimpleExtentNdims());
        dataspace.getSimpleExtentDims(&dims[0]);

        size_t n_points=dims[0];

        if (!quiet)
            cout << filename << " " << n_points << " data points" << endl;

        hsize_t offset[1]={0};

        // Read the file data points one 'membuf' chunk at a time
        while (n_points) {
            const size_t MEMBUF_SIZE=65536;
            data_point_t  membuf[MEMBUF_SIZE];

            // how many to read this time
            size_t n = (n_points<MEMBUF_SIZE)? n_points : MEMBUF_SIZE;

            // Read n data points
            hsize_t memdim[1]={n};
            hsize_t count[1]={n};
            hsize_t stride[1]={1};
            hsize_t block[1]={1};
            DataSpace memspace(1, memdim);
            dataspace.selectHyperslab(H5S_SELECT_SET, count, offset, stride, block);

            dataset.read(membuf, PredType::NATIVE_DOUBLE,memspace,dataspace);

            // Verify if the points read match
            for (int k=0;k<n;k++) {
                data_point_t x     = membuf[k];

                data_point_t x_ref = generate(j);

                if (do_transform)
                    x_ref = transform(j,x_ref);

                 if (x!=x_ref) {
                    static bool do_print=true;
                    mismatch_count++;

                    if (do_print) {
                        if (mismatch_count<30) {
                            cerr << "ERROR: mismatch at data point " << j << " (file " << filename << "): "
                                 << x << " instead of " << x_ref << endl;
                        } else {
                            cerr << "ERROR: more mismatches not printed ..." << endl;
                            do_print=false;
                        }
                    }
                }
            }

            j++;


            offset[0] += n;
            n_points  -= n;

        } // loop reading one file

    } // all files

    if (mismatch_count==0)
        cout << "Verification succesful" << endl;
    else
        cout << "ERROR: " << mismatch_count << " mismatches found" << endl;
    return 0;
}
