// core/engine/memory_manager.cpp
#include "memory_manager.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

namespace NexusForge::Core {

// Pool Allocator Implementation
template<size_t BlockSize, size_t BlockCount>
PoolAllocator<BlockSize, BlockCount>::PoolAllocator() : freeCount_(BlockCount) {
    // Initialize free list
    freeList_ = memory_;

    uint8_t* current = memory_;
    for (size_t i = 0; i < BlockCount - 1; ++i) {
        uint8_t* next = current + BlockSize;
        *reinterpret_cast<void**>(current) = next;
        current = next;
    }
    *reinterpret_cast<void**>(current) = nullptr;
}

template<size_t BlockSize, size_t BlockCount>
PoolAllocator<BlockSize, BlockCount>::~PoolAllocator() = default;

template<size_t BlockSize, size_t BlockCount>
void* PoolAllocator<BlockSize, BlockCount>::allocate() {
    std::lock_guard<std::mutex> lock(mutex_);

    if (!freeList_) return nullptr;

    void* block = freeList_;
    freeList_ = *reinterpret_cast<void**>(freeList_);
    freeCount_--;

    return block;
}

template<size_t BlockSize, size_t BlockCount>
void PoolAllocator<BlockSize, BlockCount>::deallocate(void* ptr) {
    if (!ptr) return;

    std::lock_guard<std::mutex> lock(mutex_);

    *reinterpret_cast<void**>(ptr) = freeList_;
    freeList_ = ptr;
    freeCount_++;
}

// Explicit template instantiations
template class PoolAllocator<32, 4096>;
template class PoolAllocator<64, 2048>;
template class PoolAllocator<128, 1024>;
template class PoolAllocator<256, 512>;
template class PoolAllocator<512, 256>;

// Arena Allocator Implementation
ArenaAllocator::ArenaAllocator(size_t size) : capacity_(size), used_(0) {
    memory_ = static_cast<uint8_t*>(std::aligned_alloc(64, size));
}

ArenaAllocator::~ArenaAllocator() {
    std::free(memory_);
}

void* ArenaAllocator::allocate(size_t size, size_t alignment) {
    size_t alignedOffset = (used_ + alignment - 1) & ~(alignment - 1);

    if (alignedOffset + size > capacity_) {
        return nullptr; // Out of memory
    }

    void* ptr = memory_ + alignedOffset;
    used_ = alignedOffset + size;

    return ptr;
}

void ArenaAllocator::reset() {
    used_ = 0;
}

// Memory Manager Implementation
MemoryManager::MemoryManager() : heapMemory_(nullptr), totalMemory_(0) {
    for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
        tagMemoryUsage_[i].store(0);
    }
}

MemoryManager::~MemoryManager() {
    shutdown();
}

bool MemoryManager::initialize(size_t heapSize) {
    totalMemory_ = heapSize;

    // Allocate main heap
    heapMemory_ = static_cast<uint8_t*>(std::aligned_alloc(64, heapSize));
    if (!heapMemory_) {
        std::cerr << "Failed to allocate heap memory: " << heapSize << " bytes" << std::endl;
        return false;
    }

    // Initialize pool allocators
    pool32_ = std::make_unique<PoolAllocator<32, 4096>>();
    pool64_ = std::make_unique<PoolAllocator<64, 2048>>();
    pool128_ = std::make_unique<PoolAllocator<128, 1024>>();
    pool256_ = std::make_unique<PoolAllocator<256, 512>>();
    pool512_ = std::make_unique<PoolAllocator<512, 256>>();

    // Initialize frame arena (16MB)
    frameArena_ = std::make_unique<ArenaAllocator>(16 * 1024 * 1024);

    return true;
}

void MemoryManager::shutdown() {
    // Detect leaks before shutdown
    detectLeaks();

    frameArena_.reset();
    pool512_.reset();
    pool256_.reset();
    pool128_.reset();
    pool64_.reset();
    pool32_.reset();

    if (heapMemory_) {
        std::free(heapMemory_);
        heapMemory_ = nullptr;
    }
}

void* MemoryManager::allocate(size_t size, MemoryTag tag, size_t alignment,
                               const char* file, int line) {
    // Try pool allocation first for small sizes
    void* poolPtr = allocateFromPool(size);
    if (poolPtr) {
        usedMemory_.fetch_add(size);
        tagMemoryUsage_[static_cast<size_t>(tag)].fetch_add(size);
        allocationCount_++;

        size_t current = usedMemory_.load();
        size_t peak = peakMemory_.load();
        while (current > peak && !peakMemory_.compare_exchange_weak(peak, current));

        return poolPtr;
    }

    // Fall back to general allocation
    size_t totalSize = sizeof(MemoryBlockHeader) + alignment + size;
    void* rawMemory = std::aligned_alloc(alignment, totalSize);

    if (!rawMemory) {
        std::cerr << "Memory allocation failed: " << size << " bytes" << std::endl;
        return nullptr;
    }

    // Setup header
    MemoryBlockHeader* header = static_cast<MemoryBlockHeader*>(rawMemory);
    header->size = size;
    header->tag = tag;
    header->alignment = static_cast<uint16_t>(alignment);
    header->checksum = 0xDEADBEEF;
    header->file = file;
    header->line = line;

    // Track allocation
    trackAllocation(header);

    // Update statistics
    usedMemory_.fetch_add(size);
    tagMemoryUsage_[static_cast<size_t>(tag)].fetch_add(size);
    allocationCount_++;

    size_t current = usedMemory_.load();
    size_t peak = peakMemory_.load();
    while (current > peak && !peakMemory_.compare_exchange_weak(peak, current));

    return reinterpret_cast<uint8_t*>(rawMemory) + sizeof(MemoryBlockHeader);
}

void* MemoryManager::reallocate(void* ptr, size_t newSize) {
    if (!ptr) return allocate(newSize);

    MemoryBlockHeader* header = reinterpret_cast<MemoryBlockHeader*>(
        static_cast<uint8_t*>(ptr) - sizeof(MemoryBlockHeader)
    );

    if (header->checksum != 0xDEADBEEF) {
        std::cerr << "Memory corruption detected in reallocate!" << std::endl;
        return nullptr;
    }

    if (newSize <= header->size) {
        return ptr; // No need to reallocate
    }

    void* newPtr = allocate(newSize, header->tag, header->alignment);
    if (newPtr) {
        std::memcpy(newPtr, ptr, header->size);
        deallocate(ptr);
    }

    return newPtr;
}

void MemoryManager::deallocate(void* ptr) {
    if (!ptr) return;

    MemoryBlockHeader* header = reinterpret_cast<MemoryBlockHeader*>(
        static_cast<uint8_t*>(ptr) - sizeof(MemoryBlockHeader)
    );

    if (header->checksum != 0xDEADBEEF) {
        std::cerr << "Memory corruption detected in deallocate!" << std::endl;
        return;
    }

    // Update statistics
    usedMemory_.fetch_sub(header->size);
    tagMemoryUsage_[static_cast<size_t>(header->tag)].fetch_sub(header->size);

    // Untrack allocation
    untrackAllocation(header);

    // Clear checksum to detect double-free
    header->checksum = 0;

    std::free(header);
}

void* MemoryManager::allocateFromPool(size_t size) {
    if (size <= 32) return pool32_->allocate();
    if (size <= 64) return pool64_->allocate();
    if (size <= 128) return pool128_->allocate();
    if (size <= 256) return pool256_->allocate();
    if (size <= 512) return pool512_->allocate();
    return nullptr;
}

void MemoryManager::deallocateToPool(void* ptr, size_t size) {
    if (size <= 32) { pool32_->deallocate(ptr); return; }
    if (size <= 64) { pool64_->deallocate(ptr); return; }
    if (size <= 128) { pool128_->deallocate(ptr); return; }
    if (size <= 256) { pool256_->deallocate(ptr); return; }
    if (size <= 512) { pool512_->deallocate(ptr); return; }
}

void MemoryManager::trackAllocation(MemoryBlockHeader* header) {
    std::lock_guard<std::mutex> lock(trackingMutex_);

    header->next = allocListHead_;
    header->prev = nullptr;

    if (allocListHead_) {
        allocListHead_->prev = header;
    }

    allocListHead_ = header;
}

void MemoryManager::untrackAllocation(MemoryBlockHeader* header) {
    std::lock_guard<std::mutex> lock(trackingMutex_);

    if (header->prev) {
        header->prev->next = header->next;
    } else {
        allocListHead_ = header->next;
    }

    if (header->next) {
        header->next->prev = header->prev;
    }
}

size_t MemoryManager::getMemoryByTag(MemoryTag tag) const {
    return tagMemoryUsage_[static_cast<size_t>(tag)].load();
}

void MemoryManager::printMemoryReport() const {
    std::cout << "\n=== NexusForge Memory Report ===" << std::endl;
    std::cout << "Total Memory:    " << std::setw(12) << totalMemory_ << " bytes" << std::endl;
    std::cout << "Used Memory:     " << std::setw(12) << usedMemory_.load() << " bytes" << std::endl;
    std::cout << "Peak Memory:     " << std::setw(12) << peakMemory_.load() << " bytes" << std::endl;
    std::cout << "Allocations:     " << std::setw(12) << allocationCount_.load() << std::endl;
    std::cout << "\nMemory by Tag:" << std::endl;

    const char* tagNames[] = {
        "Unknown", "Core", "UI", "Editor", "TextBuffer", "SyntaxHighlight",
        "Extension", "AI", "Network", "Render", "Texture", "Font", "Audio",
        "Temp", "String", "Container"
    };

    for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
        size_t usage = tagMemoryUsage_[i].load();
        if (usage > 0) {
            std::cout << "  " << std::setw(16) << tagNames[i] << ": "
                      << std::setw(12) << usage << " bytes" << std::endl;
        }
    }
    std::cout << "================================\n" << std::endl;
}

void MemoryManager::detectLeaks() const {
    std::lock_guard<std::mutex> lock(trackingMutex_);

    if (!allocListHead_) {
        std::cout << "No memory leaks detected." << std::endl;
        return;
    }

    std::cout << "\n=== Memory Leak Report ===" << std::endl;

    MemoryBlockHeader* current = allocListHead_;
    int leakCount = 0;
    size_t totalLeaked = 0;

    while (current) {
        std::cout << "Leak #" << (++leakCount) << ": "
                  << current->size << " bytes";
        if (current->file) {
            std::cout << " at " << current->file << ":" << current->line;
        }
        std::cout << std::endl;

        totalLeaked += current->size;
        current = current->next;
    }

    std::cout << "Total: " << leakCount << " leaks, "
              << totalLeaked << " bytes" << std::endl;
    std::cout << "===========================\n" << std::endl;
}

void MemoryManager::validateHeap() const {
    std::lock_guard<std::mutex> lock(trackingMutex_);

    MemoryBlockHeader* current = allocListHead_;

    while (current) {
        if (current->checksum != 0xDEADBEEF) {
            std::cerr << "Heap corruption detected at " << current << std::endl;
            std::abort();
        }
        current = current->next;
    }
}

// Global memory functions
namespace GlobalMemory {
    static MemoryManager* g_memoryManager = nullptr;

    void setManager(MemoryManager* manager) {
        g_memoryManager = manager;
    }

    MemoryManager* getManager() {
        return g_memoryManager;
    }

    void* allocate(size_t size, MemoryTag tag, size_t alignment,
                   const char* file, int line) {
        if (g_memoryManager) {
            return g_memoryManager->allocate(size, tag, alignment, file, line);
        }
        return std::malloc(size);
    }

    void deallocate(void* ptr) {
        if (g_memoryManager) {
            g_memoryManager->deallocate(ptr);
        } else {
            std::free(ptr);
        }
    }
}

} // namespace NexusForge::Core
