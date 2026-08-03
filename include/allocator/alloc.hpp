#ifndef ALLOC_HPP
#define ALLOC_HPP

#include <cstddef>
#include <utility>

class Arena {
private:
    std::byte* buffer_;
    std::size_t size_;
    std::size_t index_ = 0;

public:
    Arena(size_t);
    ~Arena();
    void* allocate(std::size_t, std::size_t);
    void free();
};

template <typename T>
class ArenaAllocator {
private:
    Arena* arena;

public:
    using value_type = T;

    ArenaAllocator(Arena& a) : arena(&a) { }
    ~ArenaAllocator() { }

    T* allocate(size_t how_much) {
        return reinterpret_cast<T*>(
            arena->allocate(how_much * sizeof(T), alignof(T))
        );
    }

    template <typename U>
    U* alloc_as(size_t how_much) {
        return reinterpret_cast<U*>(
            arena->allocate(how_much * sizeof(U), alignof(U))
        );
    }

    void deallocate(T*, size_t) {}

    friend bool operator==(const ArenaAllocator& a, const ArenaAllocator& b) = default;
};

template <typename T, typename Alloc, typename... Args>
T* make(Alloc& alloc, size_t count, Args... args) {
    T* variable = alloc.template alloc_as<T>(count);
    new (variable) T(std::forward<Args>(args)...);

    return variable;
}

#endif
