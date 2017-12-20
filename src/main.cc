# include <iostream>
# include <unistd.h>
# include <cstddef>

# include "api/api.hh"

# define SIZE 1000
# define MAX_INT 1000

using namespace api;

int main() {
    DistributedAllocator::init();

    int world_rank = DistributedAllocator::world_rank;
    int world_size = DistributedAllocator::world_size;

    if (DistributedAllocator::world_rank == 0)
        DistributedAllocator::alloc(SIZE);

    MPI_Barrier(MPI_COMM_WORLD);

    for (int i = 0; i < SIZE; i++)
    {
        if (i == world_rank * (MAX_INT / world_size))
            i += (MAX_INT / world_size);
        DistributedAllocator::write_at(0, i, i);
    }
    
    MPI_Barrier(MPI_COMM_WORLD);

    if (DistributedAllocator::world_rank == 0)
    {
        for (int i = 0; i < SIZE; ++i)
            std::cout << i << "\t" << DistributedAllocator::at(0, i) << "\n";
    }

    DistributedAllocator::close();
    return 0;
}
