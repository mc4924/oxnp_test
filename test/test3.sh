rm data/*

# How many seconds to run
N=20

obj/setup
obj/generator --seconds $N &
(sleep 1 && obj/reader --tag rdrA --buf RING_BUFFER1 --id 0 --seconds $N --size 2000000) &
(sleep 2 && obj/transformer --inbuf RING_BUFFER1 --id 1 --outbuf RING_BUFFER2 --seconds $N)&
(sleep 3 && obj/reader --tag rdrB --buf RING_BUFFER2  --seconds $N --size 0)
obj/setup --remove


echo ===== VERIFICATION =====

obj/verify_results data/rdrA-testdata-*

obj/verify_results --transform data/rdrB-testdata-*
