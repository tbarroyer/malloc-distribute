# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();
    int head;
    int idx;
    if (DistributedAllocator::world_rank == 1)
    {
        DistributedAllocator::alloc();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    unsigned int size = 4;
    if (DistributedAllocator::world_rank == 0) {
        head = DistributedAllocator::alloc(size);
        DistributedAllocator::write(head, 0);
        idx = head;
        for (unsigned int i = 1; i < size; i++)
        {
            idx = DistributedAllocator::next(idx);
            DistributedAllocator::write(idx, i * 10);
            std::cout << i << std::endl;
        }
        idx = head;
        std::cout << "__________________________" << std::endl;
        int val;
        for (unsigned int i = 1; i < size; i++)
        {
            idx = DistributedAllocator::next(idx);
            val = DistributedAllocator::read(idx);
            std::cout << val << std::endl;
        }
        std::cout << head << std::endl;
        std::cout << "__________________________DONE" << std::endl;
        DistributedAllocator::free(head);
        std::cout << "__________________________DONE" << std::endl;
        head = DistributedAllocator::alloc(size);

        std::cout << "__________________________DONE" << std::endl;
    }

    std::cout << "READY TO FREE" << std::endl;

    DistributedAllocator::close();
    return 0;
}
