# include <iostream>
# include <unistd.h>
# include <cstddef>

# include "api/api.hh"

# define SIZE 10
# define MAX_INT 10

using namespace api;

int main() {
    DistributedAllocator::init();

    int world_rank = DistributedAllocator::world_rank;
    int world_size = DistributedAllocator::world_size;

    if (DistributedAllocator::world_rank == 0)
        DistributedAllocator::alloc(SIZE);

    MPI_Barrier(MPI_COMM_WORLD);

    int start = world_rank * (MAX_INT / world_size);
    int stop = (world_rank + 1) * (MAX_INT / world_size);

    for (int i = start; i < stop; i++)
        DistributedAllocator::write_at(0, i, world_rank);
    
    MPI_Barrier(MPI_COMM_WORLD);

    if (DistributedAllocator::world_rank == 0)
    {
        for (int i = 0; i < SIZE; ++i)
        {
            std::cout << DistributedAllocator::at(0, i) << "\n";
        }
    }

    DistributedAllocator::close();
    return 0;
}
