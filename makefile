all: chef saladmaker

chef: chef.o shareddata.o
	g++ chef.o shareddata.o -o chef -pthread -lpthread

saladmaker: saladmaker.o shareddata.o
	g++ saladmaker.o shareddata.o -o saladmaker -pthread -lpthread

chef.o: chef.cpp
	g++ -c -pthread -lpthread chef.cpp

saladmaker.o: saladmaker.cpp
	g++ -c -pthread -lpthread saladmaker.cpp

shareddata.o: shareddata.cpp
	g++ -c shareddata.cpp

clean: 
	rm *.o chef saladmaker

# target: dependencies			#STRUCTURE OF A MAKEFILE
# 	action