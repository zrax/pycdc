CXX = g++
CXXFLAGS = -g -Wall -O2

COMMON = \
	out/module.o \
	out/data.o \
	out/bytecode.o \
	out/object.o \
	out/numeric.o \
	out/code.o \
	out/sequence.o \
	out/string.o

ALL = \
	bin/pycdas

PREFIX = /usr/local

all: $(ALL)

clean:
	rm -f $(COMMON)

install:
	mkdir -p $(PREFIX)/bin
	cp $(ALL) $(PREFIX)/bin

bin/pycdas: pycdas.cpp $(COMMON)
	$(CXX) $(CXXFLAGS) $(COMMON) pycdas.cpp -o $@

out/module.o: module.h module.cpp
	$(CXX) $(CXXFLAGS) -c module.cpp -o $@

out/data.o: data.h data.cpp
	$(CXX) $(CXXFLAGS) -c data.cpp -o $@

out/bytecode.o: bytecode.h bytecode.cpp
	$(CXX) $(CXXFLAGS) -c bytecode.cpp -o $@

out/object.o: object.h object.cpp
	$(CXX) $(CXXFLAGS) -c object.cpp -o $@

out/numeric.o: numeric.h numeric.cpp
	$(CXX) $(CXXFLAGS) -c numeric.cpp -o $@

out/code.o: code.h code.cpp
	$(CXX) $(CXXFLAGS) -c code.cpp -o $@

out/sequence.o: sequence.h sequence.cpp
	$(CXX) $(CXXFLAGS) -c sequence.cpp -o $@

out/string.o: string.h string.cpp
	$(CXX) $(CXXFLAGS) -c string.cpp -o $@
