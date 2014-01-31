# Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
# for the detailed license.

CXXFLAGS = -std=c++0x -g -pthread
SORTOBJS = sorter.o sorterimpl.o diskrun.o
LINKFLAGS = -L. -lsort -lpthread

ALLTESTS = inmemory1 assert1

.PHONY: alltests
alltests: $(ALLTESTS)
	for test in $(ALLTESTS) ;  do  ./$$test;  done

libsort.a: $(SORTOBJS) 
	$(AR) -rv $@ $^

sorter.o: sorter.h sorterimpl.h
sorterimpl.o: sorter.h sorterimpl.h runstate.h
diskrun.o: diskrun.h sortassert.h

clean:
	@rm -f *.o $(ALLTESTS) *.a

veryclean: clean
	@rm -f *~

GTEST_DIR= ../gtest
GTEST_INC = -I$(GTEST_DIR)/include
GTEST_CXXFLAGS = $(CXXFLAGS) $(GTEST_INC) -c
GTEST_LINKFLAGS = $(LINKFLAGS) -L$(GTEST_DIR)/build -lgtest_main -lgtest

inmemory1: inmemory1.o libsort.a
	$(CXX) $^ $(GTEST_LINKFLAGS) -o $@
inmemory1.o: inmemory1.cpp sorter.h
	$(CXX) $(GTEST_CXXFLAGS) -o $@ $< 

assert1: assert1.o libsort.a
	$(CXX) $^ $(GTEST_LINKFLAGS) -o $@
assert1.o: assert1.cpp sortassert.h
	$(CXX) $(GTEST_CXXFLAGS) -o $@ $< 
