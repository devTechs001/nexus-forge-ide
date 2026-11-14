// core/engine/memory_manager.cpp
#include "memory_manager.hpp"
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <iomanip>

namespace NexusForge::Core {

// Explicit template instantiations (definitions live in memory_manager.hpp)
template class PoolAllocator<32, 4096>;
template class PoolAllocator<64, 2048>;
template class PoolAllocator<128, 1024>;
template class PoolAllocator<256, 512>;
template class PoolAllocator<512, 256>;

// Arena Allocator Implementation
ArenaAllocator::ArenaAllocator(size_t size) : rawMemory_(nullptr), memory_(nullptr), capacity_(size), used_(0) {
    if (size > 0) {
        rawMemory_ = std::malloc(size + 64);
        if (rawMemory_) {
            uintptr_t addr = reinterpret_cast<uintptr_t>(rawMemory_);
            uintptr_t aligned = (addr + 63) & ~static_cast<uintptr_t>(63);
            memory_ = reinterpret_cast<uint8_t*>(aligned);
        }
    }
}

ArenaAllocator::~ArenaAllocator() {
    std::free(rawMemory_);
    rawMemory_ = nullptr;
    memory_ = nullptr;
}

void* ArenaAllocator::allocate(size_t size, size_t alignment) {
    if (!memory_) return nullptr;
    if (alignment == 0 || (alignment & (alignment - 1)) != 0) {
        alignment = alignof(std::max_align_t);
    }
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
MemoryManager::MemoryManager()
    : heapMemory_(nullptr), totalMemory_(0), usedMemory_(0), peakMemory_(0) {
    for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
        tagMemoryUsage_[i].store(0);
    }
}

MemoryManager::~MemoryManager() {
    shutdown();
}

bool MemoryManager::initialize(size_t heapSize) {
    if (heapMemory_) {
        shutdown();
    }

    totalMemory_ = heapSize;
    usedMemory_.store(0);
    peakMemory_.store(0);
    allocationCount_.store(0);
    for (size_t i = 0; i < static_cast<size_t>(MemoryTag::Count); ++i) {
        tagMemoryUsage_[i].store(0);
    }

    // Allocate main heap (rounded to 64-byte alignment)
    size_t alignedHeapSize = ((heapSize + 63) / 64) * 64;
    if (alignedHeapSize == 0) alignedHeapSize = 64;
    heapMemory_ = static_cast<uint8_t*>(std::malloc(alignedHeapSize));
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

    // Set global memory manager
    GlobalMemory::setManager(this);

    return true;
}

void MemoryManager::shutdown() {
    if (!heapMemory_ && !pool32_ && !frameArena_) {
        return;
    }

    // Clear global pointer first so nothing allocates through a dying manager
    if (GlobalMemory::getManager() == this) {
        GlobalMemory::setManager(nullptr);
    }

    // Detect leaks before shutdown
    detectLeaks();

    // Free any remaining tracked heap allocations cleanly
    {
        std::lock_guard<std::mutex> lock(trackingMutex_);
        MemoryBlockHeader* current = allocListHead_;
        while (current) {
            MemoryBlockHeader* next = current->next;
            void* raw = current->rawPtr;
            current->checksum = 0;
            std::free(raw);
            current = next;
        }
        allocListHead_ = nullptr;
    }

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
    if (size == 0) size = 1;

    // Normalize alignment to a power of two >= alignof(MemoryBlockHeader)
    size_t normalizedAlign = alignof(MemoryBlockHeader);
    while (normalizedAlign < alignment) {
        normalizedAlign <<= 1;
    }

    size_t tagIndex = static_cast<size_t>(tag);
    if (tagIndex >= static_cast<size_t>(MemoryTag::Count)) {
        tag = MemoryTag::Unknown;
        tagIndex = 0;
    }

    // Try pool allocation first for small sizes with standard alignment
    if (alignment <= alignof(std::max_align_t) && size <= 512) {
        void* poolPtr = allocateFromPool(size);
        if (poolPtr) {
            if (pool32_ && pool32_->owns(poolPtr)) pool32_->setBlockMetadata(poolPtr, size, tag);
            else if (pool64_ && pool64_->owns(poolPtr)) pool64_->setBlockMetadata(poolPtr, size, tag);
            else if (pool128_ && pool128_->owns(poolPtr)) pool128_->setBlockMetadata(poolPtr, size, tag);
            else if (pool256_ && pool256_->owns(poolPtr)) pool256_->setBlockMetadata(poolPtr, size, tag);
            else if (pool512_ && pool512_->owns(poolPtr)) pool512_->setBlockMetadata(poolPtr, size, tag);

            usedMemory_.fetch_add(size);
            tagMemoryUsage_[tagIndex].fetch_add(size);
            allocationCount_++;

            size_t current = usedMemory_.load();
            size_t peak = peakMemory_.load();
            while (current > peak && !peakMemory_.compare_exchange_weak(peak, current));

            return poolPtr;
        }
    }

    // Fall back to general aligned heap allocation with header placed directly before user pointer
    size_t totalSize = sizeof(MemoryBlockHeader) + normalizedAlign + size;
    void* rawMemory = std::malloc(totalSize);

    if (!rawMemory) {
        std::cerr << "Memory allocation failed: " << size << " bytes" << std::endl;
        return nullptr;
    }

    uintptr_t rawAddr = reinterpret_cast<uintptr_t>(rawMemory);
    uintptr_t userAddr = (rawAddr + sizeof(MemoryBlockHeader) + normalizedAlign - 1) & ~(normalizedAlign - 1);

    // Setup header immediately before aligned user pointer
    MemoryBlockHeader* header = reinterpret_cast<MemoryBlockHeader*>(userAddr - sizeof(MemoryBlockHeader));
    header->size = size;
    header->tag = tag;
    header->alignment = static_cast<uint16_t>(normalizedAlign);
    header->checksum = 0xDEADBEEF;
    header->file = file;
    header->line = line;
    header->rawPtr = rawMemory;
    header->next = nullptr;
    header->prev = nullptr;

    // Track allocation
    trackAllocation(header);

    // Update statistics
    usedMemory_.fetch_add(size);
    tagMemoryUsage_[tagIndex].fetch_add(size);
    allocationCount_++;

    size_t current = usedMemory_.load();
    size_t peak = peakMemory_.load();
    while (current > peak && !peakMemory_.compare_exchange_weak(peak, current));

    return reinterpret_cast<void*>(userAddr);
}

void* MemoryManager::reallocate(void* ptr, size_t newSize) {
    if (!ptr) return allocate(newSize);
    if (newSize == 0) {
        deallocate(ptr);
        return nullptr;
    }

    // Check if ptr belongs to a pool allocator
    size_t blockSize = 0;
    size_t oldAllocSize = 0;
    MemoryTag oldTag = MemoryTag::Unknown;
    if (findPoolBlock(ptr, blockSize, oldAllocSize, oldTag)) {
        if (newSize <= blockSize) {
            // Update tracked size in place
            if (newSize > oldAllocSize) {
                size_t delta = newSize - oldAllocSize;
                usedMemory_.fetch_add(delta);
                tagMemoryUsage_[static_cast<size_t>(oldTag)].fetch_add(delta);
            } else if (oldAllocSize > newSize) {
                size_t delta = oldAllocSize - newSize;
                usedMemory_.fetch_sub(delta);
                tagMemoryUsage_[static_cast<size_t>(oldTag)].fetch_sub(delta);
            }
            if (pool32_ && pool32_->owns(ptr)) pool32_->setBlockMetadata(ptr, newSize, oldTag);
            else if (pool64_ && pool64_->owns(ptr)) pool64_->setBlockMetadata(ptr, newSize, oldTag);
            else if (pool128_ && pool128_->owns(ptr)) pool128_->setBlockMetadata(ptr, newSize, oldTag);
            else if (pool256_ && pool256_->owns(ptr)) pool256_->setBlockMetadata(ptr, newSize, oldTag);
            else if (pool512_ && pool512_->owns(ptr)) pool512_->setBlockMetadata(ptr, newSize, oldTag);
            return ptr;
        }
        void* newPtr = allocate(newSize, oldTag, alignof(std::max_align_t));
        if (newPtr) {
            std::memcpy(newPtr, ptr, oldAllocSize);
            deallocate(ptr);
        }
        return newPtr;
    }

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

    void* newPtr = allocate(newSize, header->tag, header->alignment, header->file, header->line);
    if (newPtr) {
        std::memcpy(newPtr, ptr, header->size);
        deallocate(ptr);
    }

    return newPtr;
}

void MemoryManager::deallocate(void* ptr) {
    if (!ptr) return;

    // Handle pool-allocated blocks first
    if (deallocateIfPool(ptr)) {
        return;
    }

    MemoryBlockHeader* header = reinterpret_cast<MemoryBlockHeader*>(
        static_cast<uint8_t*>(ptr) - sizeof(MemoryBlockHeader)
    );

    if (header->checksum != 0xDEADBEEF) {
        std::cerr << "Memory corruption detected in deallocate!" << std::endl;
        return;
    }

    // Update statistics
    usedMemory_.fetch_sub(header->size);
    size_t tagIndex = static_cast<size_t>(header->tag);
    if (tagIndex < static_cast<size_t>(MemoryTag::Count)) {
        tagMemoryUsage_[tagIndex].fetch_sub(header->size);
    }

    // Untrack allocation
    untrackAllocation(header);

    // Clear checksum to detect double-free
    void* raw = header->rawPtr;
    header->checksum = 0;

    std::free(raw);
}

bool MemoryManager::findPoolBlock(const void* ptr, size_t& blockSize, size_t& allocSize, MemoryTag& tag) const {
    if (pool32_ && pool32_->getBlockMetadata(ptr, allocSize, tag)) { blockSize = 32; return true; }
    if (pool64_ && pool64_->getBlockMetadata(ptr, allocSize, tag)) { blockSize = 64; return true; }
    if (pool128_ && pool128_->getBlockMetadata(ptr, allocSize, tag)) { blockSize = 128; return true; }
    if (pool256_ && pool256_->getBlockMetadata(ptr, allocSize, tag)) { blockSize = 256; return true; }
    if (pool512_ && pool512_->getBlockMetadata(ptr, allocSize, tag)) { blockSize = 512; return true; }
    return false;
}

bool MemoryManager::deallocateIfPool(void* ptr) {
    size_t blockSize = 0;
    size_t allocSize = 0;
    MemoryTag tag = MemoryTag::Unknown;
    if (!findPoolBlock(ptr, blockSize, allocSize, tag)) {
        return false;
    }

    if (allocSize > 0) {
        usedMemory_.fetch_sub(allocSize);
        size_t tagIndex = static_cast<size_t>(tag);
        if (tagIndex < static_cast<size_t>(MemoryTag::Count)) {
            tagMemoryUsage_[tagIndex].fetch_sub(allocSize);
        }
    }

    deallocateToPool(ptr, blockSize);
    return true;
}

void* MemoryManager::allocateFromPool(size_t size) {
    if (size <= 32 && pool32_) return pool32_->allocate();
    if (size <= 64 && pool64_) return pool64_->allocate();
    if (size <= 128 && pool128_) return pool128_->allocate();
    if (size <= 256 && pool256_) return pool256_->allocate();
    if (size <= 512 && pool512_) return pool512_->allocate();
    return nullptr;
}

void MemoryManager::deallocateToPool(void* ptr, size_t size) {
    if (!ptr) return;
    if (size <= 32 && pool32_) { pool32_->deallocate(ptr); return; }
    if (size <= 64 && pool64_) { pool64_->deallocate(ptr); return; }
    if (size <= 128 && pool128_) { pool128_->deallocate(ptr); return; }
    if (size <= 256 && pool256_) { pool256_->deallocate(ptr); return; }
    if (size <= 512 && pool512_) { pool512_->deallocate(ptr); return; }
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
