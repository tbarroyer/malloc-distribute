# pragma once

# include <mpi.h>
# include <cstddef>

namespace api {
    class DistributedAllocator {
    public:
        // init OPENMPI
        static void init();

        // close OPENMPI
        static void close();

        // allocate space for an integer
        // return id of the interger
        static unsigned int alloc();

    public:
        // Number of process
        static int world_size;
        // Id of process
        static int world_rank;

    private:
        static unsigned int max_id;
    };
}
