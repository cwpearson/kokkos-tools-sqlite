#include <Kokkos_Core.hpp>

int main(void) {
  Kokkos::initialize();
  { Kokkos::Profiling::markEvent("test event!"); }
  Kokkos::finalize();
}