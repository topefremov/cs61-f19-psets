#define M61_DISABLE 1
#include "m61.hh"
#include <cstdlib>
#include <cstring>
#include <cstdio>
#include <cinttypes>
#include <cassert>
#include <limits>

static unsigned long long active_size = 0;
static unsigned long long ntotal = 0;
static unsigned long long total_size = 0;
static unsigned long long nfail = 0;
static unsigned long long fail_size = 0;
static unsigned long long nfree = 0;
static uintptr_t min_heap = std::numeric_limits<uintptr_t>::max();
static uintptr_t max_heap = 0;
m61_alloc_metadata* head = nullptr;
m61_alloc_metadata* tail = nullptr;

/// m61_malloc(sz, file, line)
///    Return a pointer to `sz` bytes of newly-allocated dynamic memory.
///    The memory is not initialized. If `sz == 0`, then m61_malloc must
///    return a unique, newly-allocated pointer value. The allocation
///    request was at location `file`:`line`.

void* m61_malloc(size_t sz, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    uintptr_t payload_ptr;
    m61_alloc_metadata metadata = { .payload_ptr = nullptr, .size = sz, .magic = MAGIC, 
                                    .next = nullptr, .prev = nullptr, .file = file, .line = line };
    size_t new_size = sz + sizeof(m61_alloc_metadata) + sizeof(unsigned char);
    void* ptr;

    if (new_size < sz) {
        // over
        ptr = nullptr;
    } else {
        ptr = base_malloc(new_size);
    }

    if (ptr == nullptr) {
        ++nfail;
        fail_size += sz;
        return ptr;
    } else {
        payload_ptr = ((uintptr_t) ptr) + sizeof(m61_alloc_metadata);
        metadata.payload_ptr = (void*) payload_ptr;
        memcpy(ptr, &metadata, sizeof(m61_alloc_metadata));
        memcpy((void*) (((uintptr_t) ptr) + sizeof(m61_alloc_metadata) + sz), &TRAILING_MAGIC, sizeof(unsigned char));
        if (payload_ptr < min_heap) {
            min_heap = payload_ptr;
        }
        m61_alloc_metadata* metadata_ptr = (m61_alloc_metadata*) ptr;
        if (head != nullptr) {
            metadata_ptr->next = head;
            head->prev = metadata_ptr;
            head = metadata_ptr;
        } else {
            head = metadata_ptr;
            tail = metadata_ptr;
        }
        uintptr_t highest_address = payload_ptr + sz;
        if (highest_address > max_heap) {
            max_heap = highest_address;
        }
        ++ntotal;
        active_size += sz;
        total_size += sz;    
    }
    return (void*) payload_ptr;
}


/// m61_free(ptr, file, line)
///    Free the memory space pointed to by `ptr`, which must have been
///    returned by a previous call to m61_malloc. If `ptr == NULL`,
///    does nothing. The free was called at location `file`:`line`.

void m61_free(void* ptr, const char* file, long line) {
    (void) file, (void) line;   // avoid uninitialized variable warnings
    if (ptr != nullptr) {
        if ((uintptr_t) ptr < min_heap || (uintptr_t) ptr > max_heap) {
            fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, not in heap\n", file, line, ptr);
            abort();
        }

        void* real_ptr = (void*) ((uintptr_t) ptr - sizeof(m61_alloc_metadata));
        m61_alloc_metadata* metadata_ptr = (m61_alloc_metadata*) real_ptr;
        if (((uintptr_t) real_ptr & 7) != 0 || metadata_ptr->magic != MAGIC || metadata_ptr->payload_ptr != ptr 
            || (metadata_ptr->prev != nullptr && metadata_ptr->prev->magic != MAGIC)
            || (metadata_ptr->next != nullptr && metadata_ptr->next->magic != MAGIC)) {
            fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, not allocated\n", file, line, ptr);
            m61_alloc_metadata* cur = head;
            uintptr_t fptr = (uintptr_t) ptr; 
            while (cur) {
                uintptr_t payload_ptr = (uintptr_t) cur->payload_ptr;
                if (fptr > payload_ptr && fptr <= (payload_ptr + cur->size)) {
                    fprintf(stderr, "  %s:%ld: %p is %ld bytes inside a %ld byte region allocated here\n", cur->file, cur->line, ptr, 
                        (fptr - payload_ptr), cur->size);
                    break;
                }
                cur = cur->next; 
            }
            abort();   
        }
        size_t free_size = metadata_ptr->size;
        if ((metadata_ptr->prev == nullptr && metadata_ptr != head) 
            || (metadata_ptr->next == nullptr && metadata_ptr != tail)
            || (metadata_ptr->prev != nullptr && metadata_ptr->prev->next != metadata_ptr)
            || (metadata_ptr->next != nullptr && metadata_ptr->next->prev != metadata_ptr)) {
            fprintf(stderr, "MEMORY BUG: %s:%ld: invalid free of pointer %p, double free\n", file, line, ptr);
            abort();
        }
        unsigned char* trailing_magic = (unsigned char*) ((uintptr_t) ptr + metadata_ptr->size);
        if (*trailing_magic != TRAILING_MAGIC) {
            fprintf(stderr, "MEMORY BUG: %s:%ld: detected wild write during free of pointer %p\n", file, line, ptr);
            abort();
        }

        if (metadata_ptr->next) {
            metadata_ptr->next->prev = metadata_ptr->prev;
        } else {
            tail = metadata_ptr->prev;
        }
        if (metadata_ptr->prev) {
            metadata_ptr->prev->next = metadata_ptr->next;
        } else {
            head = metadata_ptr->next;
        }
        base_free(real_ptr);
        active_size -= free_size;
         ++nfree;
    }
}


/// m61_calloc(nmemb, sz, file, line)
///    Return a pointer to newly-allocated dynamic memory big enough to
///    hold an array of `nmemb` elements of `sz` bytes each. If `sz == 0`,
///    then must return a unique, newly-allocated pointer value. Returned
///    memory should be initialized to zero. The allocation request was at
///    location `file`:`line`.

void* m61_calloc(size_t nmemb, size_t sz, const char* file, long line) {
    void* ptr = nullptr;
    if (mult_ok(nmemb, sz)) {
        ptr = m61_malloc(nmemb * sz, file, line);
        if (ptr) {
            memset(ptr, 0, nmemb * sz);
        }
    } else {
        nfail++;
        fail_size += nmemb * sz;
    }    
    return ptr;
}


/// m61_get_statistics(stats)
///    Store the current memory statistics in `*stats`.

void m61_get_statistics(m61_statistics* stats) {
    stats->nactive = ntotal - nfree;
    stats->ntotal = ntotal;
    stats->nfail = nfail;
    stats->active_size = active_size;
    stats->total_size = total_size;
    stats->fail_size = fail_size;
    stats->heap_max = max_heap;
    stats->heap_min = min_heap;
}


/// m61_print_statistics()
///    Print the current memory statistics.

void m61_print_statistics() {
    m61_statistics stats;
    m61_get_statistics(&stats);

    printf("alloc count: active %10llu   total %10llu   fail %10llu\n",
           stats.nactive, stats.ntotal, stats.nfail);
    printf("alloc size:  active %10llu   total %10llu   fail %10llu\n",
           stats.active_size, stats.total_size, stats.fail_size);
}


/// m61_print_leak_report()
///    Print a report of all currently-active allocated blocks of dynamic
///    memory.

void m61_print_leak_report() {
    m61_alloc_metadata* cur = head;
    while (cur) {
        void* payload_ptr = (void*) ((uintptr_t) cur + sizeof(m61_alloc_metadata));
        printf("LEAK CHECK: %s:%ld: allocated object %p with size %lu\n", cur->file, cur->line, payload_ptr, cur->size);
        cur = cur->next;
    }
}


/// m61_print_heavy_hitter_report()
///    Print a report of heavily-used allocation locations.

void m61_print_heavy_hitter_report() {
    // Your heavy-hitters code here
}

bool mult_ok(size_t x, size_t y) {
    size_t r = x * y;
    return !x || r / x == y;
}
