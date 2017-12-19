# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    if (DistributedAllocator::world_rank == 0)
    {
        for (int i = 0; i < 10; ++i)
            std::cout << DistributedAllocator::alloc() << "\n";
        std::cout << "_________________" << std::endl;

        for (int i = 0; i < 10; ++i)
        {
            std::cout << "Free " << i << std::endl;
            DistributedAllocator::free(i);
        }
        std::cout << "_________________" << std::endl;

        for (int i = 0; i < 10; ++i)
            std::cout << DistributedAllocator::alloc() << "\n";
    }

    DistributedAllocator::close();
    return 0;
}
