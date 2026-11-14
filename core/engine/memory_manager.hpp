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
    void* rawPtr;
    MemoryBlockHeader* next;
    MemoryBlockHeader* prev;
};

// Pool allocator for fixed-size allocations
template<size_t BlockSize, size_t BlockCount>
class PoolAllocator {
    static_assert(BlockSize >= sizeof(void*), "PoolAllocator BlockSize must hold a pointer");
    static_assert(BlockCount > 0, "PoolAllocator BlockCount must be > 0");
public:
    PoolAllocator() : freeCount_(BlockCount) {
        freeList_ = memory_;
        uint8_t* current = memory_;
        for (size_t i = 0; i < BlockCount - 1; ++i) {
            uint8_t* next = current + BlockSize;
            *reinterpret_cast<void**>(current) = next;
            current = next;
        }
        *reinterpret_cast<void**>(current) = nullptr;
    }

    ~PoolAllocator() = default;

    void* allocate() {
        std::lock_guard<std::mutex> lock(mutex_);
        if (!freeList_) return nullptr;
        void* block = freeList_;
        freeList_ = *reinterpret_cast<void**>(freeList_);
        freeCount_--;
        return block;
    }

    void deallocate(void* ptr) {
        if (!ptr || !owns(ptr)) return;
        std::lock_guard<std::mutex> lock(mutex_);
        size_t idx = static_cast<size_t>(reinterpret_cast<uint8_t*>(ptr) - memory_) / BlockSize;
        blockSizes_[idx] = 0;
        blockTags_[idx] = MemoryTag::Unknown;
        *reinterpret_cast<void**>(ptr) = freeList_;
        freeList_ = ptr;
        freeCount_++;
    }

    bool owns(const void* ptr) const {
        if (!ptr) return false;
        const auto addr = reinterpret_cast<uintptr_t>(ptr);
        const auto start = reinterpret_cast<uintptr_t>(memory_);
        const auto end = start + sizeof(memory_);
        return addr >= start && addr < end && ((addr - start) % BlockSize == 0);
    }

    void setBlockMetadata(void* ptr, size_t size, MemoryTag tag) {
        if (!owns(ptr)) return;
        std::lock_guard<std::mutex> lock(mutex_);
        size_t idx = static_cast<size_t>(reinterpret_cast<uint8_t*>(ptr) - memory_) / BlockSize;
        blockSizes_[idx] = size;
        blockTags_[idx] = tag;
    }

    bool getBlockMetadata(const void* ptr, size_t& size, MemoryTag& tag) const {
        if (!owns(ptr)) return false;
        std::lock_guard<std::mutex> lock(mutex_);
        size_t idx = static_cast<size_t>(reinterpret_cast<const uint8_t*>(ptr) - memory_) / BlockSize;
        size = blockSizes_[idx];
        tag = blockTags_[idx];
        return true;
    }

    size_t getFreeCount() const { return freeCount_.load(); }
    size_t getCapacity() const { return BlockCount; }
    static constexpr size_t getBlockSize() { return BlockSize; }

private:
    alignas(64) uint8_t memory_[BlockSize * BlockCount];
    size_t blockSizes_[BlockCount]{};
    MemoryTag blockTags_[BlockCount]{};
    void* freeList_;
    std::atomic<size_t> freeCount_;
    mutable std::mutex mutex_;
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
    void* rawMemory_;
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
    std::atomic<size_t> allocationCount_{0};

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
    bool findPoolBlock(const void* ptr, size_t& blockSize, size_t& allocSize, MemoryTag& tag) const;
    bool deallocateIfPool(void* ptr);
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
        if (auto* mgr = getManager()) {
            return mgr->create<T>(tag, std::forward<Args>(args)...);
        }
        return new T(std::forward<Args>(args)...);
    }

    template<typename T>
    void destroy(T* ptr) {
        if (!ptr) return;
        if (auto* mgr = getManager()) {
            mgr->destroy(ptr);
        } else {
            delete ptr;
        }
    }
}

} // namespace NexusForge::Core
