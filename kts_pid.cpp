#include "kts_pid.hpp"

#include <string>

#if defined(KTS_ENABLE_MPI)
#include <mpi.h>
#endif

#if defined(KTS_HAVE_GETPID)
#include <unistd.h>
#endif

static int kts_mpi_rank_impl() {
    int rank = 0;
#if defined(KTS_ENABLE_MPI)
    int initialized;
    MPI_Initialized(&initialized);
    if (initialized) {
        MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    }
    return rank;
#else
    {
        const char * ompiCommWorldRank = std::getenv("OMPI_COMM_WORLD_RANK");
        if (ompiCommWorldRank && std::string_view(ompiCommWorldRank) != "") {
            rank = std::atoi(ompiCommWorldRank);
            return rank;
        }
    }
#if defined(KTS_HAVE_GETPID)
    {
        rank = getpid();
        return rank;
    }
#endif // KTS_HAVE_GETPID
#endif // KTS_ENABLE_MPI
}

int kts_mpi_rank() {
    static bool once = true;
    static int rank;
    if (once) {
        rank = kts_mpi_rank_impl();
        once = false;
    }
    return rank;
}