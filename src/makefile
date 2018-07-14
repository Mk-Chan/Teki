ENGINE_NAME = Teki
UCI_NAME = "\"$(ENGINE_NAME)\""

CXX = g++
CC = gcc
NVXX = nvcc

CCFLAGS = -std=c11 -Wall -pipe -O3 -DNDEBUG
CXXFLAGS = -std=c++17 -Wall -pipe $(EXTRACXXFLAGS) -DNAME=$(UCI_NAME)
NVFLAGS = -Xptxas -O3 -DNDEBUG

LDFLAGS = -pthread -lz -Wl,--no-as-needed $(CXXFLAGS) $(EXTRALDFLAGS)
NVLDFLAGS = -L/opt/cuda/lib64 -lcuda -lcudart -lcudnn -lcublas

OBJS = main.o uci.o lookups.o position.o movegen.o move.o evaluate.o options.o\
	   test.o tbprobe.o mcts.o
NNOBJS = neural/commandline.o neural/encoder.o neural/factory.o\
		 neural/filesystem.posix.o neural/loader.o neural/network_cudnn.o\
		 neural/optionsdict.o

BINDIR = /usr/local/bin

EXE = teki

ifeq ($(BUILD),test)
	CXXFLAGS += -O3 -DNDEBUG -DTEST
else
ifeq ($(BUILD),stats)
	CXXFLAGS += -O3 -DDEBUG
else
ifeq ($(BUILD),debug)
	CXXFLAGS += -g -fno-omit-frame-pointer
else
	CXXFLAGS += -O3 -DNDEBUG
endif
endif
endif

all: $(EXE)

$(EXE): $(OBJS) $(NNOBJS)
	$(CXX) -o $@ $(OBJS) $(NNOBJS) $(LDFLAGS) $(NVLDFLAGS)

tbprobe.o: syzygy/tbprobe.c
	$(CC) $(CCFLAGS) -c -o $@ $<

neural/network_cudnn.o: neural/network_cudnn.cu
	$(NVXX) $(NVFLAGS) -c -o $@ $<

install:
	-cp $(EXE) $(BINDIR)
	-strip $(BINDIR)/$(EXE)

uninstall:
	-rm -f $(BINDIR)/$(EXE)

clean:
	-rm -f $(OBJS) $(NNOBJS) $(EXE)
