# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();

    // Process 0 ask for allocation
    /*if (DistributedAllocator::world_rank == 0) {
        DistributedAllocator::alloc();
    }

    MPI_Barrier(MPI_COMM_WORLD);

    //Process 1 ask to read memory from process 0

    if (DistributedAllocator::world_rank == 1) {
        DistributedAllocator::write(0, 99);
    }
    if (DistributedAllocator::world_rank == 1) {
        DistributedAllocator::read(0);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (DistributedAllocator::world_rank == 1) {
        DistributedAllocator::free(0);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (DistributedAllocator::world_rank == 1) {
        DistributedAllocator::alloc();
    }*/
    int head;
    int idx;
    unsigned int size = 10;
    if (DistributedAllocator::world_rank == 0) {
        head = DistributedAllocator::alloc(size);
        DistributedAllocator::write(head, 0);
        idx = head;
        for (unsigned int i = 1; i < size; i++)
        {
            idx = DistributedAllocator::next(idx);
            DistributedAllocator::write(idx, i);
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

    }
    MPI_Barrier(MPI_COMM_WORLD);

    /*if (DistributedAllocator::world_rank == 1) {
        DistributedAllocator::read(0);
    }*/

    DistributedAllocator::close();
    return 0;
}
