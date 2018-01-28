ENGINE_NAME = Teki
UCI_NAME = "\"$(ENGINE_NAME)\""

CXX = clang++
CXXFLAGS = -std=c++17 -Wall -pipe $(EXTRAFLAGS) -DNAME=$(UCI_NAME)
LDFLAGS = -pthread -Wl,--no-as-needed $(CXXFLAGS)

OBJS = main.o uci.o lookups.o position.o movegen.o move.o search.o\
       evaluate.o options.o

BINDIR = /usr/local/bin

EXE = teki

ifeq ($(BUILD),debug)
	CXXFLAGS += -g -fno-omit-frame-pointer
endif

ifeq ($(BUILD),release)
	CXXFLAGS += -O3 -flto -DNDEBUG
endif

all: $(EXE)

$(EXE): $(OBJS)
	$(CXX) -o $@ $(OBJS) $(LDFLAGS)

install:
	-cp $(EXE) $(BINDIR)
	-strip $(BINDIR)/$(EXE)

uninstall:
	-rm -f $(BINDIR)/$(EXE)

clean:
	-rm -f $(OBJS) $(EXE)
