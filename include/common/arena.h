#ifndef __ARENA_H__
#define __ARENA_H__

#include <atomic>
#include <vector>
#include <cassert>
#include "common.h"

class Arena {
public:
    DISALLOW_COPY_AND_MOVE(Arena)
    Arena();
    ~Arena();

    char* Allocate(size_t bytes);
    char* AllocateAligned(size_t bytes);

    size_t MemoryUsage() const {
        return memory_usage_.load(std::memory_order_relaxed);
    }

private:
    char* AllocateImp(size_t bytes);
    char* AllocateNewBlock(size_t block_bytes);

private:
    char* alloc_ptr_;
    size_t alloc_bytes_remain_;
    std::vector<char*> blocks_;
    std::atomic<size_t> memory_usage_;
};

#endif