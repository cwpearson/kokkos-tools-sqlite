#pragma once

#include <cstdint>

namespace lib {

void init();

void finalize();

// returns a unique id
uint64_t begin_parallel_for(const char *name, const uint32_t devID);
uint64_t begin_parallel_reduce(const char *name, const uint32_t devID);
uint64_t begin_parallel_scan(const char *name, const uint32_t devID);

// accepts the return value of the corresponding begin_parallel_for
void end_parallel_region(const uint64_t kID);

void push_profile_region(const char *name);
void pop_profile_region();

void begin_deep_copy(const char *dstSpaceName, const char *dstName,
                     const void *dst_ptr, const char *srcSpaceName,
                     const char *srcName, const void *src_ptr, uint64_t size);

// returns a unique id
uint64_t begin_fence(const char *name, const uint32_t devID);

// accepts the return value of the corresponding begin_fence
void end_fence(const uint64_t kID);

void allocate_data(const char *spaceName, const char *name, void *ptr,
                   size_t size);
void deallocate_data(const char *spaceName, const char *name, void *ptr,
                     size_t size);

void profile_event(const char *name);

} // namespace lib