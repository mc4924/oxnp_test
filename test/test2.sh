rm data/rdrA*

# How many seconds to run
N=30

obj/setup
obj/generator --seconds $N &
obj/reader --tag rdrA --buf RING_BUFFER1 --id 0 --seconds $N --size 2000000
sleep 2
obj/setup --remove

echo ===== VERIFICATION =====

obj/verify_results data/rdrA-testdata-*
