#include <Kokkos_Core.hpp>

int main(void) {
    Kokkos::initialize(); {

    {
        Kokkos::View<double *> a("a", 10), b("b", 10);
        Kokkos::deep_copy(b, a);
    }
    {
        Kokkos::View<double *> a("a", 10);
        Kokkos::deep_copy(a, 17.0);
    }

    } Kokkos::finalize();
}