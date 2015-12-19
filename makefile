
all : obj/setup obj/generator obj/reader obj/transformer

clean:
	-rm obj/*

dataclean:
	-rm data/* test/octave-workspace

allclean:
	-rm obj/* data/*




UNAME=$(shell uname)
ifeq ($(UNAME),Linux)
   LIBS=-lboost_system -lboost_thread -lboost_program_options -lhdf5 -lhdf5_cpp -lrt -pthread
   LIB_DIRS=
   INC_DIRS=
else ifeq ($(UNAME),Darwin)
   LIBS=-lboost_system-mt -lboost_thread-mt -lboost_program_options-mt -lhdf5 -lhdf5_cpp
   LIB_DIRS=-L/opt/local/lib
   INC_DIRS=-I/opt/local/include
endif


# All objects are linked the same way
LINK_CMD=g++ $(LIB_DIRS) -o $@ $< $(LIBS)


obj/setup.o: setup.cpp cmn.h swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/setup: obj/setup.o makefile
	$(LINK_CMD)



obj/generator.o: generator.cpp cmn.h period_repeat.h swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/generator: obj/generator.o makefile
	$(LINK_CMD)



obj/reader.o: reader.cpp cmn.h period_repeat.h swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/reader: obj/reader.o makefile
	$(LINK_CMD)



obj/transformer.o: transformer.cpp cmn.h period_repeat.h swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/transformer: obj/transformer.o makefile
	$(LINK_CMD)



# ========================== Objects for testing =======================

obj/test_ringbuffer.o: test/test_ringbuffer.cpp swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/test_ringbuffer: obj/test_ringbuffer.o makefile
	$(LINK_CMD)



obj/verify_results.o: test/verify_results.cpp

obj/verify_results: obj/verify_results.o makefile
	$(LINK_CMD)





# All objects are compiled the same way
COMPILE_CMD=g++ -std=c++11 -I. $(INC_DIRS) -c -o $@ $<


obj/%.o: %.cpp
	# =========================================================
	$(COMPILE_CMD)

obj/%.o: test/%.cpp
	# =========================================================
	$(COMPILE_CMD)
