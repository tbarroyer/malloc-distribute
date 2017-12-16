# include <iostream>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    // Process 0 ask for allocation
    if (DistributedAllocator::world_rank == 0) {
        DistributedAllocator::alloc();  
    }

    // Process 3 ask for allocation
    if (DistributedAllocator::world_rank == 3) {
        DistributedAllocator::alloc();  
    }

    // Process 0 ask for allocation
    if (DistributedAllocator::world_rank == 0) {
        DistributedAllocator::alloc();  
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // Process 3 ask to read memory from process 0
    if (DistributedAllocator::world_rank == 3) {
        DistributedAllocator::read(0);
    }

    // Process 3 ask to read memory from process 3
    if (DistributedAllocator::world_rank == 3) {
        DistributedAllocator::read(3221225469);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    DistributedAllocator::close();
    return 0;
}
