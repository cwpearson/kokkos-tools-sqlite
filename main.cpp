//@HEADER
// ************************************************************************
//
//                        Kokkos v. 4.0
//       Copyright (2022) National Technology & Engineering
//               Solutions of Sandia, LLC (NTESS).
//
// Under the terms of Contract DE-NA0003525 with NTESS,
// the U.S. Government retains certain rights in this software.
//
// Part of Kokkos, under the Apache License v2.0 with LLVM Exceptions.
// See https://kokkos.org/LICENSE for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//@HEADER

#include <cstdio>
#include <inttypes.h>
#include <vector>
#include <string>
#include <limits>
#include <cstring>

#include "kts.hpp"

struct SpaceHandle {
  char name[64];
};

extern "C" void kokkosp_init_library(const int loadSeq,
                                     const uint64_t interfaceVer,
                                     const uint32_t /*devInfoCount*/,
                                     void* /*deviceInfo*/) {


  lib::init();
}

extern "C" void kokkosp_finalize_library() {
  lib::finalize();
}

extern "C" void kokkosp_begin_parallel_for(const char* name,
                                           const uint32_t devID,
                                           uint64_t* kID) {
  *kID = lib::begin_parallel_for(name, devID);
}

extern "C" void kokkosp_end_parallel_for(const uint64_t kID) {
  lib::end_parallel_for(kID);
}


extern "C" void kokkosp_push_profile_region(char* regionName) {
  lib::push_profile_region(regionName);
}

extern "C" void kokkosp_pop_profile_region() {
  lib::pop_profile_region();
}

extern "C" void kokkosp_begin_deep_copy(SpaceHandle dst_handle,
                                        const char* dst_name,
                                        const void* dst_ptr,
                                        SpaceHandle src_handle,
                                        const char* src_name,
                                        const void* src_ptr, uint64_t size) {
  lib::begin_deep_copy(dst_handle.name, dst_name, dst_ptr, src_handle.name, src_name, src_ptr, size);
}
