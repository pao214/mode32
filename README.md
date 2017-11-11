# Problem Statement

To support memory accesses higher than 4GB limit we need to use 64 bit addressing of memory. This transition comes with a limitation. Now fewer pointers can fit in cache memory owing to increased size of pointers. In fact there is observed performance increase in certain applications when 32 bit pointers are used instead of 64 bit pointers. We to use 32 bit pointers whenever possile and switch to using 64 bit pointers only on demand and this switch needs to happen transparently.

# Objectives

As a first proof of concept we want to show performance improvement completely at the application level without going into the internal details of the compiler. To do this we need a reduced problem that can be implemented efficiently at application level.

## Reduced Problem Statement

### Stack and Global Memory
Since cpp does not directly provide direct access to operations on stack memory, implementing any switch algorithm on stack/global memory would be very difficult or impossible. So this application only tries to optimize pointers in heap memory.

### Structures and Primitive data types

Since we cannot extract any information regarding the structures declared in the user code of any fairly high level language, we need the user to manually enter the information regarding any structures he later uses. We also cannot store or extract any identity information regarding primitive data types since they are abstracted out by the compiler. Hence we also need to define primitive data types separately as structures. As a result there is lack of type checking and several other features.

### Pointer and Object Model

We have a restricted pointer model and took inspiration from JAVA semantics which does not have the notion of pointers but simply references. Although this higher abstraction is more restrictive, it allows relatively easier solution to the problem. In particular we have the following minimum restrictions that result in undefined behaviour if violated by the user.

1. Pointer should point to objects similar to references in JAVA. Switch algorithm used assumes this to be true in particular while updating pointers.

2. Fields in structures can only have single indirections or need to be a primitive type. More indirections or non-primitive types are not supported as fields but can be implemented using another structure.

## Switch Timing

To make a fair evaluation, we compare execution in 32 bit mode of the program with 64 bit mode of the same program. So, although the memory footprint of the program is below 4GB, we make a switch after recording execution time of 32 bit program and then record execution time of the 64 bit version.

# Implementation

We initialize structures, construct a reverse point to graph, initialize memory, execute benchmark in 32 bit mode, switch from 32 bit mode to 64 bit mode and execute benchmark in 64 bit mode.

## Memory Allocator

### Buddy Allocator

We allocate a page aligned memory of required size and abstract out an interface to allocate memory through buddy style allocation at the lowest level. Minimum granularity of allocation is page size and allocates contiguous aligned memory in powers of 2 multiples of page size. 

### Slab Allocator

Abstraction layer on top of buddy allocator. Allocates objects based on the type. Internally the alocator allocates objects of same type in the same contiguous memory called slab allocated from buddy allocator. Each slab contains a free list from which object allocations can be served. Arrays can also be allocated using the API.

## Switch Algorithm

The algorithm has the following minimum requirements

1. The fast path of accessing and updating pointers should not take extra time to execute.

2. Overhead of allocation and deallocation should not be significant in comparison to the original time.

3. Must successfully update the references to reallocated memory for 64 bit pointers.

4. Algorithm must be fast enough not to have heavy latency.

Algorithm is designed keeping these things in mind. The trick is to convert any 32 bit pointer to 64 bit counter part in O(1).

The implementation follows the following steps in sequence

1. Relocate memory.

2. Update stack pointers.

3. Copy objects from old to new memory. Care must be taken to copy pointers correctly.

4. Updates the free list in the new memory.

5. Frees all the old memory which is unnecessary.


# Evaluation

The program is run on simple microbenchmark that just does a linked list traversal. We allocate 2^16 nodes contianing pointers to itself and create a circular linked list out of it. We then traverse the linked list 2^30 times in both 32 bit mode and 64 bit mode and compare the execution times.

32 bit mode: ~7 million cycles
switch: ~10 thousand cycles
64 bit mode: ~10 million cycles