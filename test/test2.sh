RAEDER_TAG=test2-rdr

# Remove any relevant data files
rm data/$RAEDER_TAG* 2>/dev/null

# How many seconds to run
N=15

# Run the test. Note that the 'generator' is run in the background
# (in parallel with the reader)
obj/setup
obj/generator --seconds $N &
obj/reader --tag $RAEDER_TAG --buf RING_BUFFER1 --id 0 --seconds $N --size 2000000
obj/setup --remove


echo ===== VERIFICATION =====

obj/verify_results data/$RAEDER_TAG*
