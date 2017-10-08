ENGINE_NAME = Teki
UCI_NAME = "\"$(ENGINE_NAME)\""

CXX = g++
CXXFLAGS = -std=c++11 -Wall -O3 -flto -pipe $(EXTRAFLAGS)
LDFLAGS = -Wl,--no-as-needed $(CXXFLAGS)

OBJS = main.o

BINDIR = /usr/local/bin

EXE = teki

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
