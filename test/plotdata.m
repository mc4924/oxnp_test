function plotdata(N)
d=[];
for k=0:N
    load('-hdf5', sprintf('../data/testdata%03d.h5',k));
    d=[d dset];
end
plot(d,'.');