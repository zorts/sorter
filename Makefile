CXXFLAGS = -std=c++0x -g 

driver: driver.o sorter.o sorterimpl.o
	$(CXX) $(CXXFLAGS) -o driver $^
driver.o: sorter.h keyconvert.h

sorter.o: sorter.h sorterimpl.h
sorterimpl.o: sorter.h sorterimpl.h runstate.h

clean:
	@rm -f *.o driver

veryclean: clean
	@rm -f *~