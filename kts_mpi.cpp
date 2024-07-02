#include "kts_mpi.hpp"

#include <string>

#if defined(KTS_ENABLE_MPI)
#include <mpi.h>
#endif

int kts_mpi_rank() {
    int rank = 0;
#if defined(KTS_ENABLE_MPI)
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    return rank;
#else
    {
        const char * ompiCommWorldRank = std::getenv("OMPI_COMM_WORLD_RANK");
        if (ompiCommWorldRank) {
            rank = std::atoi(ompiCommWorldRank);
        }
    }
    return rank;
#endif
}