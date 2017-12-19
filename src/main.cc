# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();
    int head;
 //   int idx;
    /*if (DistributedAllocator::world_rank == 1)
    {
        DistributedAllocator::alloc();
    }*/
    unsigned int size = 10;
    if (DistributedAllocator::world_rank == 0) {
        head = DistributedAllocator::alloc(size);
        std::cout << "_________________________" << std::endl;
        /*DistributedAllocator::write(head, 0);
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
        std::cout << head << std::endl;*/
        MPI_Barrier(MPI_COMM_WORLD);
        DistributedAllocator::free(head);
        MPI_Barrier(MPI_COMM_WORLD);
        std::cout << "_________________________" << std::endl;
        //head = DistributedAllocator::alloc(size);
        std::cout << "_________________________" << std::endl;
        //DistributedAllocator::free(head);
        head = DistributedAllocator::alloc(size);
        
    }
    MPI_Barrier(MPI_COMM_WORLD);

    DistributedAllocator::close();
    return 0;
}
