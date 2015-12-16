
(generator) ===> [ringbuffer] ===> (reader) ===> data/testdataXXX.h5


generator.cpp
-------------
Process that generates N data points per seconds and puts them in a ring buffer in
shared memory.



reader.cpp
----------
Process that reads the ring buffer and puts the data into HDF5 files.



cmn.h
-----
Common defintions for generator and reader.



swmr_ringbuffer.h
swmr_ringbuffer.cpp
-------------------
The Single Writer-Multiple Reader ring buffer implementation.
