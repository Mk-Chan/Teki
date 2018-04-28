ENGINE_NAME = Teki
UCI_NAME = "\"$(ENGINE_NAME)\""

CXX = g++
CXXFLAGS = -std=c++17 -Wall -pipe $(EXTRACXXFLAGS) -DNAME=$(UCI_NAME)
LDFLAGS = -pthread -Wl,--no-as-needed $(CXXFLAGS) $(EXTRALDFLAGS)

OBJS = main.o uci.o lookups.o position.o movegen.o move.o search.o\
       evaluate.o options.o tbprobe.o

BINDIR = /usr/local/bin

EXE = teki

ifeq ($(BUILD),debug)
	CXXFLAGS += -g -fno-omit-frame-pointer
else
	CXXFLAGS += -O3 -DNDEBUG
endif

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

tbprobe.o: syzygy/tbprobe.c
	$(CXX) $(CXXFLAGS)   -c -o $@ $<

install:
	-cp $(EXE) $(BINDIR)
	-strip $(BINDIR)/$(EXE)

uninstall:
	-rm -f $(BINDIR)/$(EXE)

clean:
	-rm -f $(OBJS) $(EXE)
