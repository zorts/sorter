CXXFLAGS = -std=c++0x -g 

driver: driver.o sorter.o
	$(CXX) $(CXXFLAGS) -o driver $^

sorter.o: sorter.h 
driver.o: sorter.h keyconvert.h
