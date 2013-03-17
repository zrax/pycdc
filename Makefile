CXX = g++
CXXFLAGS = -g -Wall -Wextra -Werror
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
	python_10 python_11 python_13 python_14 python_15 python_16 \
	python_20 python_21 python_22 python_23 python_24 \
	python_25 python_26 python_27 \
	python_30 python_31 python_32 python_33

BYTE_OBJS = $(BYTES:%=out/%.o)
BYTE_SRCS = $(BYTES:%=bytes/%.cpp)
BYTE_MAPS = $(BYTES:%=bytes/%.map)

ALL = \
	bin/pycdas \
	bin/pycdc

PREFIX = /usr/local/pycdc

all: $(ALL)

clean:
	rm -f $(COMMON) $(BYTE_OBJS) $(BYTE_SRCS)
	rm -f tests/*.pyc.src tests/*.pyc.err

install:
	mkdir -p $(PREFIX)/bin
	cp $(ALL) $(PREFIX)/bin

test: all
	@fails=0; \
	files=(); \
	errors=(); \
	for f in ./tests/*.pyc; \
	do \
		stderr=$$( ./bin/pycdc "$$f" 2>$$f.err 1>$$f.src ); \
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

bin/pycdas: pycdas.cpp $(COMMON) $(BYTE_OBJS)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $^ -o $@

bin/pycdc: pycdc.cpp $(COMMON) $(BYTE_OBJS)
	$(CXX) $(CXXFLAGS) $(LFLAGS) $^ -o $@

out/%.o: %.cpp %.h
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

out/python_%.o: bytes/python_%.cpp $(BYTE_SRCS)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -c $< -o $@

$(BYTE_SRCS): $(BYTE_MAPS)
	( cd bytes ; ./comp_map.py )
