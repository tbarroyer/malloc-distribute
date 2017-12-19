# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    if (DistributedAllocator::world_rank == 0)
    {
        for (int i = 0; i < 6; ++i)
            std::cout << DistributedAllocator::alloc() << "\n";
        
        for (int i = 0; i < 6; ++i)
            DistributedAllocator::free(i);

        for (int i = 0; i < 6; ++i)
            std::cout << DistributedAllocator::alloc() << "\n";
    }

    DistributedAllocator::close();
    return 0;
}
