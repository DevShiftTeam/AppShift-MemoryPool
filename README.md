# MemoryPool For C++
A fast cross-platform memory pool mechanism for C++, about 8-9 times faster than new &amp; delete operators.
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
To use the memory pool features you just need to copy the [MemoryPool.cpp](MemoryPool.cpp) & [MemoryPool.h](MemoryPool.h) files to your project. ***The Memory Pool Is Not Thread Safe - In case of threads it is better to create a memory pool for each thread***

 * _Create a memory pool_: `CPPShift::Memory::MemoryPool mp = CPPShift::Memory::MemoryPool();` Should be created to use the features.
 * _Allocate space_: `Type* allocated = mp->allocate<Type>(instances);` Where `Type` is the object\primitive type to create and `instances` is a size_t representing the amount of `Type`s to create.
 * _Dellocate space_: `mp->remove(allocated)` Remove an allocated space
 * _Rellocate space_: `Type* allocated = mp->rellocate<Type>(allocated, instances);` Rellocate a pre-allocated space, will copy the previous values to the new space.

## Macros
There are some helpful macros available to indicate how you want the MemoryPool to manage your memory allocations.
 * `#define MEMORYPOOL_BLOCK_MAX_SIZE 1024 * 1024`: The MemoryPool allocates memory into blocks, each block can have a maximum size avalable to use - when it exceeds this size, the MemoryPool allocates a new block - use this macro to define the maximum size to give to each block. By default the value is `1024 * 1024` which is 1MB.
 * `#define MEMORYPOOL_REUSE_GARBAGE`: Deleted space is not really deleted, it is only flagged as 'deleted' - to reuse deleted instances then define this macro and when allocations are requested the memory pool will search for the first big enough deleted block - decreases memory consumption but might effect performance depending on amount of deleted units and how far they are in memory from each other.
 * `#define MEMORYPOOL_IGNORE_MAX_BLOCK_SIZE`: Sometime we might want to allocate more space than the maximum size defined for a block. By default MemoryPool throws an exception with the value `EMemoryErrors::EXCEEDS_MAX_SIZE` - to allow the creation of a bigger block if necessary, then define this macro.

# Methodology
The MemoryPool creates a chain of blocks starting from a single block which size is defined using the `MEMORYPOOL_BLOCK_MAX_SIZE` macro (See [Macros](#macros)) or the size passed into the MemoryPool constructor.

Each block contains a block header the size of 24 bytes containing the following information:
 * `size_t block_size;` - Size of the block
 * `size_t offset;` - Offset in the block from which the memory is free (The block is filled in sequencial order)
 * `SMemoryBlockHeader* next;` - Pointer to the next block

When a block is fully filled the MemoryPool create a new block and relates it to the previous block.

When allocating a space, MemoryPool creates a SMemoryUnitHeader and moves the blocks offset forward by the header size plus the amount of space requested. The header is 16 or 24 bytes long (Depending if `MEMORYPOOL_REUSE_GARBAGE` is on, see [Macros](#macros)) and contains the following data:
 * `size_t length;` - The length in bytes of the allocated space
 * `bool is_deleted;` - A flag indicating if the memory unit got deleted
 * `SMemoryUnitHeader* prev_deleted;` - Pointer to the previous deleted memory unit - only if the `MEMORYPOOL_REUSE_GARBAGE` is on.

# About
- ***Sapir Shemer*** is the proud business owner of [DevShift](devshift.biz) and an Open-Source enthusiast. Have been programming since the age of 7. Mathematics Student :)

# More to come in later versions
In the next versions I'm planning to add some interesting features:
- Ability to put thread safety on the memory pool to make a thread shared memory pool.
- Ability to create an inter-process memory pool which can be shared between different processes.
- `compressGarbage()`: Will compress deleted units that are next to eachother into one unit.
- `cleanBlocks()`: Block that contain deleted data only will be removed from the memory block chain.