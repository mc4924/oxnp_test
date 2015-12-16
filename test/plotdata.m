function plotdata(id,N)
d=[];
for k=0:N
    load('-hdf5', sprintf('../data/rdr%d-testdata-%03d.h5',id,k));
    d=[d dset];
end
plot(d,'.');