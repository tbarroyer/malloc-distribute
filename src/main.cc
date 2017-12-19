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

        for (int i = 0; i < SIZE; i++)
            DistributedAllocator::write_at(head, i, rand() % SIZE);
    }

    MPI_Barrier(MPI_COMM_WORLD);

    if (DistributedAllocator::world_rank == 0)
    {
        int head = 0;
        for (int i = 0; i < SIZE; i++)
        {
            int val = DistributedAllocator::at(head, i);
            std::cout << "Index is: " << i << " and value is " << val << std::endl;
        }
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    DistributedAllocator::close();
    return 0;
}
