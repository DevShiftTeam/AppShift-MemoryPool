# MemoryPool For C++
A very fast cross-platform memory pool mechanism for C++ built using a data-oriented approach.
I hope this simple feature will help you increase your software's performance - and there are more projects and features to come under the CPPShift library name, wait for it ;)

# Table of Contents
- [MemoryPool For C++](#memorypool-for-c)
- [Table of Contents](#table-of-contents)
- [Usage](#usage)
  - [Macros](#macros)
- [Methodology](#methodology)
- [About](#about)
- [More to come in later versions](#more-to-come-in-later-versions)


# Usage
To use the memory pool features you just need to copy the [MemoryPool.cpp](MemoryPool.cpp), [MemoryPool.h](MemoryPool.h) & [MemoryPoolData.h](MemoryPoolData.h) files to your project. The memory pool strcture is `CPPShift::Memory::MemoryPool`. ***The Memory Pool Is Not Thread Safe - In case of threads it is better to create a memory pool for each thread***

 * _Create a memory pool_: `CPPShift::Memory::MemoryPool * mp = CPPShift::Memory::MemoryPoolManager::create();` Create a new memory pool structure and a first memory block.
 * _Allocate space_: `Type* allocated = new (mp) Type[size];` Where `Type` is the object\primitive type to create, `mp` is the memory pool structure address, and `size` is a represention of the amount of types to allocate.
 * _Deallocate space_: `CPPShift::Memory::MemoryPoolManager::free(allocated)` Remove an allocated space
 * _Reallocate space_: `Type* allocated = (Type*) CPPShift::Memory::MemoryPoolManager::reallocate(allocated, size);` Rellocate a pre-allocated space, will copy the previous values to the new memory allocated.
 * _Dump data of a memory pool_: `CPPShift::Memory::MemoryPoolManager::dumpPoolData(mp)` Where `mp` is a pointer to the memory pool structure to dump. This function prints outs the data about the blocks and units in the pool.

## Macros
There are some helpful macros available to indicate how you want the MemoryPool to manage your memory allocations.
 * `#define MEMORYPOOL_DEFAULT_BLOCK_SIZE 1024 * 1024`: The MemoryPool allocates memory into blocks, each block can have a maximum size avalable to use - when it exceeds this size, the MemoryPool allocates a new block - use this macro to define the maximum size to give to each block. By default the value is `1024 * 1024` which is 1MB.
 * `#define MEMORYPOOL_REUSE_GARBAGE`: Deleted space is not really deleted, it is only flagged as 'deleted' - to reuse deleted instances then define this macro and when allocations are requested the memory pool will search for the first big enough deleted block - decreases memory consumption but might effect performance depending on amount of deleted units and how far they are in memory from each other.
 * `#define MEMORYPOOL_SAFE_ALLOCATION`: When allocating with the `new` keyword the system won't check if the memory pool is a null pointer and will trust that it is not, this way checking if the memory pool is not null the job of the developer, but it makes things faster as it is easier for the branch predictor (as I found out through various tests). If this definition is defined then the 'new' keyword will check if the memory pool is null for you and return a `nullptr`.

# Methodology
The MemoryPool is a structure pointing to the start of a chain of blocks, which size is by default `MEMORYPOOL_BLOCK_MAX_SIZE` macro (See [Macros](#macros)) or the size passed into the `CPPShift::Memory::MemoryPoolManager::create(size)` function. The MemoryPoolManager is a component holding the necessary function to work with the MemoryPool structure. What's also good is that you can also access the MemoryPool strcture directly if needed.

Each block contains a block header the size of 32 bytes containing the following information:
 * `size_t blockSize;` - Size of the block
 * `size_t offset;` - Offset in the block from which the memory is free (The block is filled in sequencial order)
 * `MemoryPool* mp_container;` - Pointer to the MemoryPool containing this block - this way it is easy to get information about the MemoryPool structure from the block itself
 * `SMemoryBlockHeader* next;` - Pointer to the next block

When a block is fully filled the MemoryPool creates a new block and relates it to the previous block.

When allocating a space, MemoryPool creates a SMemoryUnitHeader and moves the blocks offset forward by the header size plus the amount of space requested. The header is 16 or 32 bytes long (Depending if `MEMORYPOOL_REUSE_GARBAGE` is on, see [Macros](#macros)) and contains the following data:
 * `size_t length;` - The length in bytes of the allocated space
 * `SMemoryBlockHeader* container` - Block which this unit belongs to
 * `bool isDeleted;` - A flag indicating if the memory unit got deleted
 * `SMemoryUnitHeader* prevDeleted;` - Pointer to the previous deleted memory unit - only if the `MEMORYPOOL_REUSE_GARBAGE` is on.

# About
- ***Sapir Shemer*** is the proud business owner of [DevShift](devshift.biz) and an Open-Source enthusiast. Have been programming since the age of 7. Mathematics Student :)

# More to come in later versions
In the next versions I'm planning to add some interesting features:
- Ability to put thread safety on the memory pool to make a thread shared memory pool.
- Ability to create an inter-process memory pool which can be shared between different processes.
- `compressGarbage()`: Will compress deleted units that are next to eachother into one unit.
- `cleanBlocks()`: Block that contain deleted data only will be removed from the memory block chain.
