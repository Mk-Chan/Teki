ENGINE_NAME = Teki
UCI_NAME = "\"$(ENGINE_NAME)\""

CXX = g++
CXXFLAGS = -std=c++11 -Wall -pipe $(EXTRAFLAGS) -DNAME=$(UCI_NAME)
LDFLAGS = -Wl,--no-as-needed $(CXXFLAGS)

OBJS = main.o uci.o bitboard.o position.o

BINDIR = /usr/local/bin

EXE = teki

ifeq ($(BUILD),debug)
	CXXFLAGS += -g -fno-omit-frame-pointer
endif

ifeq ($(BUILD),release)
	CXXFLAGS += -O3 -flto
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
