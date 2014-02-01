# Copyright (c) 2014 by Jerry L. Callen. See the LICENSE file
# for the detailed license.

CXXFLAGS = -std=c++0x -g -pthread
SORTOBJS = sorter.o sorterimpl.o diskrun.o
LINKFLAGS = -L. -lsort -lpthread

ALLTESTS = assert1 diskrun1 runstate1

.PHONY: alltests stats
alltests: $(ALLTESTS)
	result=0; for test in $(ALLTESTS);  do ./$$test; result=$$(($$result+$$?));  done; exit $$result

libsort.a: $(SORTOBJS) 
	$(AR) -rv $@ $^

sorter.o: sorter.h sorterimpl.h
sorterimpl.o: sorter.h sorterimpl.h runstate.h
diskrun.o: diskrun.h sortassert.h

clean:
	@rm -f *.o $(ALLTESTS) *.a

veryclean: clean
	@rm -f *~

stats:
	cloc *.h *.cpp Makefile

GTEST_DIR= ../gtest
GTEST_INC = -I$(GTEST_DIR)/include
GTEST_CXXFLAGS = $(CXXFLAGS) $(GTEST_INC) -c
GTEST_LINKFLAGS = $(LINKFLAGS) -L$(GTEST_DIR)/build -lgtest_main -lgtest

assert1: assert1.o libsort.a
	$(CXX) $^ $(GTEST_LINKFLAGS) -o $@
assert1.o: assert1.cpp sortassert.h
	$(CXX) $(GTEST_CXXFLAGS) -o $@ $< 

diskrun1: diskrun1.o libsort.a
	$(CXX) $^ $(GTEST_LINKFLAGS) -o $@
diskrun1.o: diskrun1.cpp diskrun.h
	$(CXX) $(GTEST_CXXFLAGS) -o $@ $< 

runstate1: runstate1.o libsort.a
	$(CXX) $^ $(GTEST_LINKFLAGS) -o $@
runstate1.o: runstate1.cpp sorter.h
	$(CXX) $(GTEST_CXXFLAGS) -o $@ $< 

timingrunsort: timingrunsort.o libsort.a
	$(CXX) $^ $(LINKFLAGS) -o $@
timingrunsort.o: timingrunsort.cpp runstate.h sorter.h
	$(CXX) $(CXXFLAGS) -c -o $@ $< 

runtiming: timingrunsort
	./timingrunsort

# Not currently made
inmemory1: inmemory1.o libsort.a
	$(CXX) $^ $(GTEST_LINKFLAGS) -o $@
inmemory1.o: inmemory1.cpp sorter.h
	$(CXX) $(GTEST_CXXFLAGS) -o $@ $< 
