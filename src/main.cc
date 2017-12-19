# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    if (DistributedAllocator::world_rank == 0)
    {
        int head = DistributedAllocator::alloc(10);
        int idx = head;
        for (int i = 0; i < 10; i++)
        {
            std::cout << "Index is: " << idx << std::endl;
            idx = DistributedAllocator::next(idx);
        }
    }

    DistributedAllocator::close();
    return 0;
}
