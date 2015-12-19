# Remove any relevant data files
rm data/rdrA* data/rdrB* 2>/dev/null


# How many seconds to run
N=20


# Run the test. Note that the 'generator', the first reader and the transformer
# are run in the background (so taht all processes run in parallel).
# Also, note that the reader and the transformer are started with a little delay
# (just for fun)
obj/setup
obj/generator --seconds $N &
(sleep 1 && obj/reader --tag rdrA --buf RING_BUFFER1 --id 0 --seconds $N --size 2000000) &
(sleep 2 && obj/transformer --inbuf RING_BUFFER1 --id 1 --outbuf RING_BUFFER2 --seconds $N)&
(sleep 3 && obj/reader --tag rdrB --buf RING_BUFFER2  --seconds $N --size 0)
obj/setup --remove


echo ===== VERIFICATION =====

obj/verify_results data/rdrA-testdata-*

obj/verify_results --transform data/rdrB-testdata-*
