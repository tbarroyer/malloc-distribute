# include <iostream>
# include <climits>

# include "api.hh"

namespace api {
    int DistributedAllocator::world_size = 0;
    int DistributedAllocator::world_rank = 0;
    unsigned int DistributedAllocator::max_id = -1;

    void DistributedAllocator::init() {
        MPI_Init(NULL, NULL);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        max_id = world_rank * (UINT_MAX / world_size);
    }

    void DistributedAllocator::close() {
        MPI_Finalize();
    }

    unsigned int DistributedAllocator::alloc() {
        std::cout << "Process " << world_rank << " is asking for memory" << std::endl;

        std::cout << "ID " << max_id << " given" << std::endl;
        return max_id++;
    }
}
