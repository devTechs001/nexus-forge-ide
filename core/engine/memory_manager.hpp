// core/engine/memory_manager.hpp
#pragma once

#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <unordered_map>
#include <vector>
#include <atomic>
#include <string>

namespace NexusForge::Core {

// Memory allocation tags for tracking
enum class MemoryTag : uint16_t {
    Unknown = 0,
    Core,
    UI,
    Editor,
    TextBuffer,
    SyntaxHighlight,
    Extension,
    AI,
    Network,
    Render,
    Texture,
    Font,
    Audio,
    Temp,
    String,
    Container,
    Count
};

// Memory block header
struct MemoryBlockHeader {
    size_t size;
    MemoryTag tag;
    uint16_t alignment;
    uint32_t checksum;
    const char* file;
    int line;
    MemoryBlockHeader* next;
    MemoryBlockHeader* prev;
};

// Pool allocator for fixed-size allocations
template<size_t BlockSize, size_t BlockCount>
class PoolAllocator {
public:
    PoolAllocator();
    ~PoolAllocator();

    void* allocate();
    void deallocate(void* ptr);

    size_t getFreeCount() const { return freeCount_.load(); }
    size_t getCapacity() const { return BlockCount; }

private:
    alignas(64) uint8_t memory_[BlockSize * BlockCount];
    void* freeList_;
    std::atomic<size_t> freeCount_;
    std::mutex mutex_;
};

// Arena allocator for temporary allocations
class ArenaAllocator {
public:
    explicit ArenaAllocator(size_t size);
    ~ArenaAllocator();

    void* allocate(size_t size, size_t alignment = 8);
    void reset();

    size_t getUsed() const { return used_; }
    size_t getCapacity() const { return capacity_; }

private:
    uint8_t* memory_;
    size_t capacity_;
    size_t used_;
};

// Main Memory Manager
class MemoryManager {
public:
    MemoryManager();
    ~MemoryManager();

    bool initialize(size_t heapSize);
    void shutdown();

    // Allocation functions
    void* allocate(size_t size, MemoryTag tag = MemoryTag::Unknown,
                   size_t alignment = 8, const char* file = nullptr, int line = 0);
    void* reallocate(void* ptr, size_t newSize);
    void deallocate(void* ptr);

    // Tagged allocation helpers
    template<typename T, typename... Args>
    T* create(MemoryTag tag, Args&&... args) {
        void* memory = allocate(sizeof(T), tag, alignof(T));
        return new(memory) T(std::forward<Args>(args)...);
    }

    template<typename T>
    void destroy(T* ptr) {
        if (ptr) {
            ptr->~T();
            deallocate(ptr);
        }
    }

    // Pool allocation
    void* allocateFromPool(size_t size);
    void deallocateToPool(void* ptr, size_t size);

    // Arena allocation (for frame-temporary data)
    ArenaAllocator& getFrameArena() { return *frameArena_; }
    void resetFrameArena() { frameArena_->reset(); }

    // Statistics
    size_t getTotalMemory() const { return totalMemory_; }
    size_t getUsedMemory() const { return usedMemory_.load(); }
    size_t getPeakMemory() const { return peakMemory_.load(); }
    size_t getAllocationCount() const { return allocationCount_.load(); }

    // Memory usage by tag
    size_t getMemoryByTag(MemoryTag tag) const;
    void printMemoryReport() const;

    // Debugging
    void detectLeaks() const;
    void validateHeap() const;

private:
    uint8_t* heapMemory_;
    size_t totalMemory_;
    std::atomic<size_t> usedMemory_;
    std::atomic<size_t> peakMemory_;
    std::atomic<size_t> allocationCount_;

    std::unique_ptr<ArenaAllocator> frameArena_;

    // Pool allocators for common sizes
    std::unique_ptr<PoolAllocator<32, 4096>> pool32_;
    std::unique_ptr<PoolAllocator<64, 2048>> pool64_;
    std::unique_ptr<PoolAllocator<128, 1024>> pool128_;
    std::unique_ptr<PoolAllocator<256, 512>> pool256_;
    std::unique_ptr<PoolAllocator<512, 256>> pool512_;

    // Tracking
    mutable std::mutex trackingMutex_;
    MemoryBlockHeader* allocListHead_ = nullptr;
    std::atomic<size_t> tagMemoryUsage_[static_cast<size_t>(MemoryTag::Count)];

    void trackAllocation(MemoryBlockHeader* header);
    void untrackAllocation(MemoryBlockHeader* header);
};

// Global memory functions
#define NEXUS_NEW(Type, Tag, ...) \
    NexusForge::Core::GlobalMemory::create<Type>(Tag, ##__VA_ARGS__)

#define NEXUS_DELETE(ptr) \
    NexusForge::Core::GlobalMemory::destroy(ptr)

#define NEXUS_ALLOC(size, tag) \
    NexusForge::Core::GlobalMemory::allocate(size, tag, 8, __FILE__, __LINE__)

#define NEXUS_FREE(ptr) \
    NexusForge::Core::GlobalMemory::deallocate(ptr)

namespace GlobalMemory {
    void setManager(MemoryManager* manager);
    MemoryManager* getManager();

    void* allocate(size_t size, MemoryTag tag, size_t alignment,
                   const char* file, int line);
    void deallocate(void* ptr);

    template<typename T, typename... Args>
    T* create(MemoryTag tag, Args&&... args) {
        return getManager()->create<T>(tag, std::forward<Args>(args)...);
    }

    template<typename T>
    void destroy(T* ptr) {
        getManager()->destroy(ptr);
    }
}

} // namespace NexusForge::Core
