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
(sleep 1 && obj/reader --tag rdrA --buf RING_BUFFER1 --id 0 --seconds 20 --size 200000) &\
(sleep 2 && obj/transformer --inbuf RING_BUFFER1 --id 1 --outbuf RING_BUFFER2 --seconds 20)&
(sleep 3 && obj/reader --tag rdrB --buf RING_BUFFER2  --seconds 20 --size 0)&
```

will run one 'generator', one 'transformer' and two 'reader' processes for 20 seconds

```
(generator)-->[buf1] -+-->(reader A)---> data/rdrA-testdata-XXX.h5 (fixed size of 200000 data points)
                      |
                      V
                 (transformer)
                      |
                      V
                   [buf2]
                      |
                      +-->(reader B)---> data/rdrB-testdata-XXX.h5 (variable size)
```


The data from the generator is just a sequence of double floating points (the sum of two sinusoids of different
amplitude and frequency).

'readerA' just reads the data from [buffer1] and logs it in HDF5 files, all of the same length.

'transformer' modifies the data (doubles the value) from one buffer to another.

'readerB' logs the data from the transformer in files of varying (random) length.



-------------
**generator.cpp**

Process that generates N data points per seconds and puts them in a ring buffer in
shared memory.
Needs to run first because it creates the shared memory and ring buffer.
A command line parameter allows to specify for how long to run.


----------
**reader.cpp**

Process that reads the ring buffer and puts the data into HDF5 files.
Command line parameters allow to specify for how long to run, the name
of the ring buffer, the reader ID and the file size (fixed or random).

----------
**transformer.cpp**

Process that reads from one ring buffer,modifies it and puts it on another ring buffer.
Command line parameters allow to specify for how long to run, the name
of the ring buffers, the reader ID.


----------
**cmn.h**

Common definitions for generator and reader (names for shared memory structures etc)


-------------------
**swmr_ringbuffer.h**
**swmr_ringbuffer.cpp**

The Single Writer-Multiple Reader ring buffer implementation.

-----------------------

**FIXIT:**

Add proper tests:
1. fast highly concurrent accesses to the ring buffer
2. high throughput
3. drift of timing between generator and reader
4. long duration


Creator of 'swmr_ringbuffer' is not thread/process safe

Graceful handling of errors/exception

