CC = g++
CFLAGS = -c -std=c++11
DEPS = mode32.h blprnt.h vnode.h slab.h
OBJ = mode32.o user.o memory.o graph.o slab.o page.o

all: mode32

mode32: $(OBJ)
	$(CC) $^

%.o: %.cpp $(DEPS)
	$(CC) $(CFLAGS) $<

clean:
	rm *.o *.out