# $@--目标文件，$^--所有的依赖文件，$<--第一个依赖文件。
Target   = myServer
cc 	     = g++
FLAGS    = -std=c++11 -Wall
LIBS     = -lpthread
OBJECTS  = Main.o Util.o Event.o Lightning.o Http.o Timer.o Threadpool.o

$(Target) : $(OBJECTS)
	g++ $(FLAGS) $(OBJECTS) -o $(Target) $(LIBS) 

Main.o : Main.cpp Lightning.h
	g++ $(FLAGS) -c Main.cpp

Util.o : Util.cpp Util.h
	g++ $(FLAGS) -c Util.cpp

Event.o : Event.cpp Event.h Util.h Timer.h Threadpool.h
	g++ $(FLAGS) -c Event.cpp

Lightning.o : Lightning.cpp Lightning.h Event.h Util.h
	g++ $(FLAGS) -c Lightning.cpp

Http.o : Http.cpp Http.h
	g++ $(FLAGS) -c Http.cpp

Timer.o : Timer.cpp Timer.h Http.h 
	g++ $(FLAGS) -c Timer.cpp

Threadpool.o : Threadpool.cpp Threadpool.h Util.h Timer.h 
	g++ $(FLAGS) -c Threadpool.cpp $(LIBS)

.PHONY : clean veryclean

clean :
	-rm *.o 

veryclean : clean
	-rm $(Target)
