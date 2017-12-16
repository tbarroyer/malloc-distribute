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

    DistributedAllocator::close();
    return 0;
}
