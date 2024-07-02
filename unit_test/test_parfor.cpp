#include <Kokkos_Core.hpp>

int main(void) {
    Kokkos::initialize(); {
    std::stringstream ss; ss << __FILE__ << ":" << __LINE__;
    Kokkos::parallel_for(ss.str(), 10, KOKKOS_LAMBDA(int){});
    } Kokkos::finalize();
}