# include <iostream>

# include "api.hh"

namespace api {
    int DistributedAllocator::world_size = 0;
    int DistributedAllocator::world_rank = 0;

    void DistributedAllocator::init() {
        MPI_Init(NULL, NULL);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
    }

    void DistributedAllocator::close() {
        MPI_Finalize();
    }

    int DistributedAllocator::alloc() {
        return 0;
    }
}
