#include <Kokkos_Core.hpp>

int main(void) {
  Kokkos::initialize();
  {
    double res;
    std::stringstream ss;
    ss << __FILE__ << ":" << __LINE__;
    Kokkos::parallel_scan(ss.str(), 10, KOKKOS_LAMBDA(int, double &, bool){},
                          res);
  }
  Kokkos::finalize();
}