CROSS_COMPILE = 
CXX = $(CROSS_COMPILE)g++ $(CFLAGS)
CPPFLAGS = -Wall -g
AR=ar

TARGET = sample

INCLUDES = -I../../../include
LINKLIBS = -L../../../lib -lMVSDK -lrt -lpthread
SOURCES = $(wildcard *.cpp)
OBJECTS = $(patsubst %.cpp, %.o, $(SOURCES))

$(TARGET):$(OBJECTS)
	$(CXX) $(INCLUDES) $(CPPFLAGS) $(OBJECTS) $(LINKLIBS) -o $(TARGET)
	rm -f $(OBJECTS)
	
.cpp.o:
	$(CXX) $(INCLUDES) -c $(CPPFLAGS) $< -o $@
	
.PHONY: clean
clean:
	rm -f $(OBJECTS) $(TARGET)

	
