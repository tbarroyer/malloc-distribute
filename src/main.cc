# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    if (DistributedAllocator::world_rank == 0)
    {
        int head = DistributedAllocator::alloc(10);
        //MPI_Barrier(MPI_COMM_WORLD);
        int idx = head;
        for (int i = 0; i < 10; i++)
        {
            std::cout << "Index is: " << idx << std::endl;
            DistributedAllocator::write(idx, idx * 10);
            idx = DistributedAllocator::next(idx);
        }

        idx = head;
        int val = -1;
        for (int i = 0; i < 10; i++)
        {
            val = DistributedAllocator::read(idx);
            std::cout << "Index is: " << idx << " and value is " << val << std::endl;
            idx = DistributedAllocator::next(idx);
        }
    }

    DistributedAllocator::close();
    return 0;
}
