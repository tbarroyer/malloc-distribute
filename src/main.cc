# include <iostream>
# include <unistd.h>
# include <cstddef>

# include "api/api.hh"

# define SIZE 10

using namespace api;

int main() {
    DistributedAllocator::init();

    if (DistributedAllocator::world_rank == 0)
    {
        int head = DistributedAllocator::alloc(SIZE);

        MPI_Barrier(MPI_COMM_WORLD);
        int idx = head;
        for (int i = 0; i < SIZE; i++)
        {
            DistributedAllocator::write(idx, rand() % SIZE);
            idx = DistributedAllocator::next(idx);
        }
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (DistributedAllocator::world_rank == 4)
    {
        int idx = 0;
        int val = -1;
        for (int i = 0; i < SIZE; i++)
        {
            val = DistributedAllocator::read(idx);
            std::cout << "Index is: " << idx << " and value is " << val << std::endl;
            idx = DistributedAllocator::next(idx);
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    DistributedAllocator::close();
    return 0;
}
