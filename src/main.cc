# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    if (DistributedAllocator::world_rank == 0)
    {
        DistributedAllocator::alloc();
    }

    if (DistributedAllocator::world_rank == 1)
    {
        DistributedAllocator::alloc();
    }

    DistributedAllocator::close();
    return 0;
}
