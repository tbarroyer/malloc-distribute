# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    // Process 0 ask for allocation
    if (DistributedAllocator::world_rank == 0) {
        DistributedAllocator::alloc();  
    }

    // Process 2 ask for allocation
    if (DistributedAllocator::world_rank == 2) {
        DistributedAllocator::alloc();  
    }

    // Process 3 ask for allocation
    if (DistributedAllocator::world_rank == 3) {
        DistributedAllocator::alloc();  
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (DistributedAllocator::world_rank == 0) {
         DistributedAllocator::write(1610612733, 19);
     }

    MPI_Barrier(MPI_COMM_WORLD);

    // Process 3 ask to read memory from process 3
    if (DistributedAllocator::world_rank == 3) {
        DistributedAllocator::read(1610612733);
    }

    // Process 0 ask to read memory from process 3
    if (DistributedAllocator::world_rank == 0) {
        DistributedAllocator::read(1610612733);
    }


    //Process 1 ask to read memory from process 0
    if (DistributedAllocator::world_rank == 1) {
        DistributedAllocator::read(0);
    }

    DistributedAllocator::close();
    return 0;
}
