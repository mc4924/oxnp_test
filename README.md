Test code to verify interprocess exchange of data using shared memory buffers
and writing data into HDF5 files.

----------------
**Tested on:**

Linux (Ubuntu 14.04):
Requires installing BOOST and HDF5:  `apt-get install libhdf5-dev`   `apt-get install libboost-all-dev`

Mac OS X (10.11):
Requires installing BOOST and HDF5 with macports:  `port install boost hdf5`

------------------
**To build:**

make

------------------
**To run:**

On bash, this command line:
```
obj/generator --seconds 22 & \
(sleep 1 && obj/reader --id 0 --seconds 20 --size 2000) &\
(sleep 2 && obj/reader --id 1 --seconds 20 --size 0)&
```

will run one 'generator' and two 'reader' processes for 20 seconds

```
(generator)-->[buf] -+-->(reader 0)---> data/rdr0-testdata-XXX.h5 (fixed size of 2000 data points)
                     |
                     |
                     +-->(reader 1)---> data/rdr1-testdata-XXX.h5 (variable size)
```

The two reader processes will write two set of files in the data directory,containing the same
data from the generator. One set of file is of fixed size, one of variable size.

-------------
**generator.cpp**

Process that generates N data points per seconds and puts them in a ring buffer in
shared memory.
Needs to run first because it creates the shared memory and ring buffer.
A command line parameter allows to specify for how long to run.


----------
**reader.cpp**

Process that reads the ring buffer and puts the data into HDF5 files.
Command line parameters allow to specify for how long to run, the reader ID and the file size (fixed or random).


----------
**cmn.h**

Common definitions for generator and reader (names for shared memory structures etc)


-------------------
**swmr_ringbuffer.h**
**swmr_ringbuffer.cpp**

The Single Writer-Multiple Reader ring buffer implementation.

-----------------------

**FIXIT**
Creator of 'swmr_ringbuffer' is not process safe

Graceful handling of errors/exception

