# AppShift MemoryPool
<div style="
    display: flex;
    width: 100%;
    justify-content: center;
">
    <div>
        <table style="text-align: center">
            <tr>
                <td style="font-weight: bold">Title</td>
                <td>AppShift MemoryPool Library</td>
            </tr>
            <tr>
                <td style="font-weight: bold">Author</td>
                <td>Sapir Shemer</td>
            </tr>
            <tr>
                <td style="font-weight: bold">Thread Safety</td>
                <td>Lock Free + Lock Based</td>
            </tr>
            <tr>
                <td style="font-weight: bold">Platforms</td>
                <td>Cross-Platform</td>
            </tr>
        </table>
    </div>
</div>

Welcome to the most extensive open-source memory pool library!
After my experience with different and my own memory pool, I've decided to write a full-fledged library containing a
memory pool from (almost) every architecture possible for user-space pools (Unless, I've missed something?).
Each pool architecture has its own advantages and disadvantages which I shall explain in this readme briefly.
I can guarantee that there is no other more extensive set of memory pools in a single library.

The memory pools presented are all much faster than regular new/delete allocations :)

**Then why use malloc/free (new/delete)?** Malloc was developed to "do the work for you".
When you use dynamic memory, it's hard to guess in which order you will want to retrieve memory and by which sizes,
thus not managing memory allocations correctly might cause problems like
[fragmentation](https://en.wikipedia.org/wiki/Fragmentation_(computing)) that will make you run out of
memory faster than you consume it.
Malloc provides you an interface to allocate memory without thinking about stuff like fragmentation.
(Malloc is still fragmentation prone)

Memory pools are different, they are designed to be fast. In fact, they require you to think about the process of your 
allocation and de-allocation a little deeper, but in return you get a much faster and safer allocation and de-allocation
process, and as a result a faster program.
Using memory pools correctly you can solve issues of performance degradation and memory failures due to fragmentation.


# Contents
<!-- TOC -->
* [AppShift MemoryPool](#appshift-memorypool)
* [Contents](#contents)
* [Usage](#usage)
  * [Stack Pool](#stack-pool)
  * [Segregated/Object Pool](#segregatedobject-pool)
* [MemoryPool Architectures](#memorypool-architectures)
  * [Segregated vs Stack Pools](#segregated-vs-stack-pools)
  * [Limited vs Unlimited Pools](#limited-vs-unlimited-pools)
  * [Thread Safety: Lock Free vs Locks](#thread-safety-lock-free-vs-locks)
  * [Static vs Dynamic Blocks](#static-vs-dynamic-blocks)
  * [MemoryPool Scoping](#memorypool-scoping)
  * [Stack-Based: Stack, Buddy & Slab Allocators](#stack-based-stack-buddy--slab-allocators)
* [Best Practices](#best-practices)
* [Benchmarks](#benchmarks)
* [Thanks](#thanks)
<!-- TOC -->

# Usage
## Stack Pool

```c++
#include <include/stack/StackPool.h>

StackPool<100 * 1024> pool = StackPool<100 * 1024>();

// Allocate
MyVariable* var_list = new (pool) MyVariable[20];   // Same as reinterpret_cast<MyVariable*>(pool.allocate(sizeof(MyVariable) * 20))
AnotherVariable* var = new (pool) AnotherVariable;  // Same as reinterpret_cast<AnotherVariable*>(pool.allocate(sizeof(AnotherVariable)))

// Play a little...

// Free
delete var;         // Same as pool.free(var)
delete[] var_list;  // Same as pool.free(var_list)
```

By default, the stack pool is thread safe with a lock-free architectural style. If size is not specified in the template
or is 0, then the pool creates the first block dynamically and not in the stack.

* For a non-thread safe pool use `StackPool<100 * 1024, EPoolArchitecture::NON_THREAD_SAFE>`.
* For a lock based pool use `StackPool<100 * 1024, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>`.
* For dynamically created blocks use `StackPool(100 * 1024)` or similarly `StackPool<0>(100 * 1024)`.

## Segregated/Object Pool
```c++
#include <include/object/ObjectPool.h>

ObjectPool<MyVariable> pool {};

// Allocate
MyVariable* var = new (pool) MyVariable;    // Same as pool.allocate()

// Play a little...

// Free
pool.free(var);
```

By default, the object pool is thread safe with a lock free architectural style, and is created for `128` objects.
* To control the number of items in the pool use `ObjectPool<TYPE, size>` to create `size` items in each block.
* To create a non-thread safe pool use `ObjectPool<TYPE, size, EPoolArchitecture::NON_THREAD_SAFE>`.
* To create a lock based pool use `ObjectPool<TYPE, size, EPoolArchitecture::THREAD_SAFE_LOCK_BASED>`.

The `SegregatedPool` has only the first template parameter different from `ObjectPool`, where it is a value of the size
of each segment, such that for segments of size `16` bytes the corresponding segregated pool will be
`SegregatedPool<16>`:

```c++
#include <include/object/SegregatedPool.h>

SegregatedPool<sizeof(MyVariable)> pool {};

// Allocate
MyVariable* var = new (pool) MyVariable;    // Same as reinterpret_cast<MyVariable*>(pool.allocate())

// Play a little...

// Free
pool.free(var);
```

__Important and Nice feature!__ All `SegregatedPool` objects with the same item size and count use the same memory blocks!
Therefore, if two different variables use the `ObjectPool` and are the same size, and use the same item count for each block, then
the same blocks will be used for them, which makes the usage of `ObjectPool` preferable.

Behind the scenes, the `ObjectPool<TYPE>` uses `SegregatedPool<sizeof(TYPE)>`.

# MemoryPool Architectures
After reading a variety of papers, using different pools, and developing my own ideas and architectures, I've mapped the
set of (almost?) all possible architectural styles a memory pool may include, and I've created a memory pool class for each
available combination of architectures (Except for limited pools architecture since it has no performance advantage and
are less comfortable to use).

In the following titles I introduce each possible architecture, and compare it to it's opposite.

## Segregated vs Stack Pools
__Segregated pools__ are pools that are divided into segments of same sizes each.
Each allocation returns a segment of the same size, and when a segment is freed, then it holds a pointer to the
previous free segment (or nullptr if it doesn't exist).
This chain of free items is used to allocate again when allocation is called, thus allowing for fast allocations.

__Stack pools__ act like the application's stack. The pool requests a block of pre-allocated space,
then it allocated data top-down in this block. Deleted items are stored as a linked list to keep track of all the
deleted data for re-allocating.

<div style="
    display: flex;
    width: 100%;
    justify-content: center;
">
    <div>
        <table>
            <thead>
                <th>Architecture</th>
                <th>Advantages</th>
                <th>Disadvantages</th>
            </thead>
            <tr>
                <td>Segregated</td>
                <td>
<ul>
<li>
    <span style="font-weight: bold;">
        Fastest memory pool
    </span>:
    Since allocation and de-allocation are the simplest.
</li>
<li>
    <span style="font-weight: bold;">
        Fragmentation Free
    </span>:
    Fragmentation cannot happen as sizes are fixed.
</li>
</ul>
                </td>
                <td>
<ul>
<li>
    <span style="font-weight: bold;">
        Fixed-size allocation
    </span>:
    A different size requires a new pool.
</li>
</ul>
                </td>
            </tr>
            <tr>
                <td>Stack</td>
                <td>
<ul>
<li>
    <span style="font-weight: bold;">
        Faster than the OS
    </span>:
    User space operations are cheap and do not require any system calls unlike regular allocations.
</li>
<li>
    <span style="font-weight: bold;">
        Flexible sizes
    </span>:
    Can allocate any size in the pool with no need to open a new pool.
</li>
</ul>
                </td>
                <td>
<ul>
<li>
    <span style="font-weight: bold;">
        Fragmentation Prone
    </span>:
    Fragmentation can happen and might affect performance.
</li>
</ul>
                </td>
            </tr>
        </table>
    </div>
</div>

Segregated pools are faster than stack based pools.
You might prefer a stack pool in cases you need to create dynamic arrays with sizes that are deducible only at runtime,
or for any management of data structures that are only deducible at runtime, otherwise it is preferable to use a
segregated pool.

Stack based pools can have multiple types of allocation/de-allocation mechanism, each with its own advantages and disadvantages affecting fragmentation & performance, the mechanisms presented here are discussed in [Stack-Based: Stack, Buddy \& Slab Allocators](#stack-based-stack-buddy--slab-allocators).

## Limited vs Unlimited Pools
__Limited Pools__ are pools that have only a single block allocated, they do not allocate further blocks.
Once the block is full, the allocation will return an error.

__Unlimited Pools__ are pools that allocate a new block when all the space is full, this makes the pool feel "endless",
hence the name unlimited. Blocks in an unlimited pool are chained as a linked list to keep track of them.

All the pool implemented in this library are unlimited pools, they have no performance penalty since it is only a single
instruction for the check if a new block is needed, so new block creations are very rare.
The only case where one will use a limited pool is when resources are essential and an error is needed when space runs
out.

## Thread Safety: Lock Free vs Locks
__Lock Free Pools__ Create a new chain of blocks that is thread local and thus an allocation will not occur on the same
block for different threads, giving the pool thread safety capabilities.

__Lock Pools__ Use the same blocks throughout different threads, but keep thread safety by locking allocations and
de-allocations.

<div style="
    display: flex;
    width: 100%;
    justify-content: center;
">
    <div>
        <table>
            <thead>
                <th>Architecture</th>
                <th>Advantages</th>
                <th>Disadvantages</th>
            </thead>
            <tr>
                <td>Lock Free</td>
                <td>
                    Amazingly fast as they introduce no overhead.
                </td>
                <td>
                    Require more memory since a new block is created for each thread.
                </td>
            </tr>
            <tr>
                <td>Lock-Based</td>
                <td>
                    Memory efficient as they only have a single block for multiple accessing threads.
                </td>
                <td>
                    Performance penalty for using allocation locks when requesting or freeing memory.
                </td>
            </tr>
        </table>
    </div>
</div>

The implementation of the lock-based pool in this library doesn't require a lock for each allocation, but it makes a
lock on every de-allocation. This is done for performance & safety reasons. As having a lock-free pool, when you
allocate on thread-local storage, and then de-allocate on another thread without locking, might cause a race condition.

On fully lock-free pools, you must ensure that you allocate and de-allocate the same variable on the same thread,
or use the lock-based pool. If you use a non-thread safe pool, then you must ensure that you allocate and de-allocate
all variables with the same pool on the same thread.


## Static vs Dynamic Blocks
__Dynamic Blocks__ Simply mean that the first block is created dynamically in the heap.

__Static Blocks__ Means that the first block created on pool initialization will use the stack thus will be less expensive to allocate.

Those concepts only work on the first block of memory created by the pool, all other blocks will have to be dynamic.
The performance difference is negligible as this architecture only affects the initialization of the pool.


## MemoryPool Scoping


## Stack-Based: Stack, Buddy & Slab Allocators
TBD (To Be Developed)
In the next versions, I will provide a way to choose an implementation of the allocation mechanism for stack based pools.


# Best Practices
Best practices take performance, thread safety and maintainability as main principles.
Personally, I don't like the idea of best practices as everything depends on circumstances, but for most cases I assume
this will be the best choice:

* `ObjectPool<TYPE, COUNT>` and `StackPool<SIZE>` without any other parameters already implement the best practices concerning thread safety and other qualities, use a different implementation or specify additional template parameters only if required for specific situation.
* Use __lock-free__ thread safety over __lock-based__ thread safety whenever you can (unless you really care about
memory efficiency).
* Use `ObjectPool` over `StackPool` when possible, a `StackPool` is only good when you are working with data structures which sizes are evaluated at runtime, but for single objects, linked lists of the same object and similar applications should be made using an `ObjectPool`.
* Thread-safe allocations do not imply thread-safe data structures, you still need to use locks, atomic operations or thread locallity on your own requested memory to ensure thread safety.
* Use [memory scoping](#memorypool-scoping) when you can, it is a very useful feature that can help you de-allocate memory much faster and more efficiently.

# Benchmarks

# Thanks
