all : obj/generator obj/reader obj/transformer

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


obj/generator.o: generator.cpp cmn.h swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/generator: obj/generator.o makefile
	g++ $(LIB_DIRS) -o $@ $< $(LIBS)



obj/reader.o: reader.cpp cmn.h swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/reader: obj/reader.o makefile
	g++ $(LIB_DIRS) -o $@ $< $(LIBS)


obj/transformer.o: transformer.cpp cmn.h swmr_ringbuffer.h swmr_ringbuffer.cpp

obj/transformer: obj/transformer.o makefile
	g++ $(LIB_DIRS) -o $@ $< $(LIBS)

obj/%.o: %.cpp
	# =========================================================
	g++ -std=c++0x $(INC_DIRS) -c -o $@ $<
