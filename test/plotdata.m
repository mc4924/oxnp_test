function plotdata(tag,N)
d=[];
for k=0:N
    load('-hdf5', sprintf('../data/%s-testdata-%03d.h5',tag,k));
    d=[d dset];
end
plot(d);