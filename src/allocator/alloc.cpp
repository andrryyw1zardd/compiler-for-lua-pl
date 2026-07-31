#include "allocator/alloc.hpp"
#include <new>

Arena::Arena(size_t s) : size_(s) {
    buffer_ = reinterpret_cast<std::byte*>(::operator new(s));
}

Arena::~Arena() {
    ::operator delete(buffer_);
}

void* Arena::allocate(size_t s, size_t al = alignof(std::max_align_t)) {
    std::size_t current = reinterpret_cast<std::size_t>(buffer_ + index_);
    std::size_t aligned = (current + al - 1) & ~(al - 1);
    std::size_t padding = aligned - current; 

    if (index_ + s + padding > size_) 
        throw std::bad_alloc();

    index_ += s + padding;

    return reinterpret_cast<void*>(aligned);
}

void Arena::free() {
    index_ = 0;
}
