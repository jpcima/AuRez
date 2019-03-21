PREFIX ?= /usr/local
CXX ?= g++
CXXFLAGS ?= -O2 -g -Wall -std=c++11
LDFLAGS ?=

all: bin/AuRez

clean:
	rm -rf bin obj

install: all
	install -D -m 755 bin/AuRez $(DESTDIR)$(PREFIX)/bin/AuRez

.PHONY: all clean install

obj/%.o: %.cc
	@mkdir -p obj
	$(CXX) $(CXXFLAGS) -c -o $@ $<

bin/AuRez: obj/AuRez.o obj/AuRsrc.o obj/MacRsrc.o
	@mkdir -p bin
	$(CXX) $(LDFLAGS) -o $@ $^

AuRez.cc: AuRsrc.h MacRsrc.h
AuRsrc.cc: AuRsrc.h
MacRsrc.cc: MacRsrc.h
