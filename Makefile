CXX = g++
CXXFLAGS = -g -Wall
#CXXFLAGS += -fprofile-arcs -ftest-coverage
#CPPFLAGS += -DBLOCK_DEBUG -DSTACK_DEBUG
#LFLAGS += -lgcov
SHELL = /bin/bash

COMMON = \
	out/data.o \
	out/bytecode.o \
	out/pyc_module.o \
	out/pyc_object.o \
	out/pyc_numeric.o \
	out/pyc_code.o \
	out/pyc_sequence.o \
	out/pyc_string.o \
	out/ASTree.o \
	out/ASTNode.o \

BYTES = \
	out/python_10.o \
	out/python_11.o \
	out/python_13.o \
	out/python_14.o \
	out/python_15.o \
	out/python_16.o \
	out/python_20.o \
	out/python_21.o \
	out/python_22.o \
	out/python_23.o \
	out/python_24.o \
	out/python_25.o \
	out/python_26.o \
	out/python_27.o \
	out/python_30.o \
	out/python_31.o \
	out/python_32.o

ALL = \
	bin/pycdas \
	bin/pycdc

PREFIX = /usr/local

all: $(ALL)

clean:
	rm -f $(COMMON) $(BYTES)

install:
	mkdir -p $(PREFIX)/bin
	cp $(ALL) $(PREFIX)/bin

test: all
	@fails=0; \
	files=(); \
	errors=(); \
	for f in ./tests/*; \
	do \
		stderr=$$( ./bin/pycdc "$$f" 2>&1 1>/dev/null ); \
		if [ "$$?" -eq "0" -a -z "$$stderr" ]; then \
			echo -ne "\033[32m.\033[m"; \
		else \
			let fails+=1; \
			files=("$${files[@]}" "$$f"); \
			errors=("$${errors[@]}" "$$stderr"); \
			echo -ne "\033[31m.\033[m"; \
		fi \
	done; \
	echo -e "\n\n$$fails tests failed:"; \
	for ((i=0; i<$${#files[@]}; i++)); \
	do \
		echo -e "\t\033[31m$${files[i]}\033[m"; \
	echo -e "$${errors[i]}\n"; \
	done;

bin/pycdas: pycdas.cpp $(COMMON) $(BYTES)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $(COMMON) $(BYTES) pycdas.cpp -o $@

bin/pycdc: pycdc.cpp $(COMMON) $(BYTES)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $(COMMON) $(BYTES) pycdc.cpp -o $@

out/%.o: %.cpp %.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

out/python_%.o: bytes/python_%.cpp
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

bytes/python_%.cpp: bytes/python_%.map
	( cd bytes ; ./comp_map.py )
