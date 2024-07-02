#include <Kokkos_Core.hpp>

int main(void) {
    Kokkos::initialize(); {
    Kokkos::View<double *> a("a", 10);
    } Kokkos::finalize();
}