#pragma once

#include <cstdint>

namespace lib {


void init();

void finalize();



// returns a unique id
uint64_t begin_parallel_for(const char* name, const uint32_t devID);

// accepts the return value of the corresponding begin_parallel_for
void end_parallel_for(const uint64_t kID);

} // namespace lib