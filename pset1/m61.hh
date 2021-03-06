#ifndef M61_HH
#define M61_HH 1
#include <cassert>
#include <cstdlib>
#include <cinttypes>
#include <cstdio>
#include <new>

constexpr unsigned int MAGIC = 0xBEEF;
constexpr unsigned char TRAILING_MAGIC = '@';


/// m61_malloc(sz, file, line)
///    Return a pointer to `sz` bytes of newly-allocated dynamic memory.
void* m61_malloc(size_t sz, const char* file, long line);

/// m61_free(ptr, file, line)
///    Free the memory space pointed to by `ptr`.
void m61_free(void* ptr, const char* file, long line);

/// m61_calloc(nmemb, sz, file, line)
///    Return a pointer to newly-allocated dynamic memory big enough to
///    hold an array of `nmemb` elements of `sz` bytes each. The memory
///    should be initialized to zero.
void* m61_calloc(size_t nmemb, size_t sz, const char* file, long line);


/// m61_statistics
///    Structure tracking memory statistics.
struct m61_statistics {
    unsigned long long nactive;         // # active allocations
    unsigned long long active_size;     // # bytes in active allocations
    unsigned long long ntotal;          // # total allocations
    unsigned long long total_size;      // # bytes in total allocations
    unsigned long long nfail;           // # failed allocation attempts
    unsigned long long fail_size;       // # bytes in failed alloc attempts
    uintptr_t heap_min;                 // smallest allocated addr
    uintptr_t heap_max;                 // largest allocated addr
};

// Structure tracking per-allocation metadata.
struct m61_alloc_metadata {
    void* payload_ptr;          // pointer to payload
    size_t size;                // size of an allocated block
    unsigned int magic;         // denote if the block is allocated by malloc
    m61_alloc_metadata* next;   // next allocated block in the list
    m61_alloc_metadata* prev;   // previous allocated block in the list
    const char* file;           // file name where malloc was called 
    long line;                  // line number where malloc was called
};

/// m61_get_statistics(stats)
///    Store the current memory statistics in `*stats`.
void m61_get_statistics(m61_statistics* stats);

/// m61_print_statistics()
///    Print the current memory statistics.
void m61_print_statistics();

/// m61_print_leak_report()
///    Print a report of all currently-active allocated blocks of dynamic
///    memory.
void m61_print_leak_report();

/// m61_print_heavy_hitter_report()
///    Print a report of heavily-used allocation locations.
void m61_print_heavy_hitter_report();

/// `m61.cc` should use these functions rather than malloc() and free().
void* base_malloc(size_t sz);
void base_free(void* ptr);
void base_allocator_disable(bool is_disabled);
bool mult_ok(size_t x, size_t y);


/// Override system versions with our versions.
#if !M61_DISABLE
#define malloc(sz)          m61_malloc((sz), __FILE__, __LINE__)
#define free(ptr)           m61_free((ptr), __FILE__, __LINE__)
#define calloc(nmemb, sz)   m61_calloc((nmemb), (sz), __FILE__, __LINE__)
#endif


/// This magic class lets standard C++ containers use your debugging allocator,
/// instead of the system allocator.
template <typename T>
class m61_allocator {
public:
    using value_type = T;
    m61_allocator() noexcept = default;
    m61_allocator(const m61_allocator<T>&) noexcept = default;
    template <typename U> m61_allocator(m61_allocator<U>&) noexcept {}

    T* allocate(size_t n) {
        return reinterpret_cast<T*>(m61_malloc(n * sizeof(T), "?", 0));
    }
    void deallocate(T* ptr, size_t) {
        m61_free(ptr, "?", 0);
    }
};
template <typename T, typename U>
inline constexpr bool operator==(const m61_allocator<T>&, const m61_allocator<U>&) {
    return true;
}
template <typename T, typename U>
inline constexpr bool operator!=(const m61_allocator<T>&, const m61_allocator<U>&) {
    return false;
}

#endif
