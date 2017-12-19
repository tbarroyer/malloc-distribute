# include <iostream>
# include <unistd.h>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();
    sleep(0.5);
    int head;
    //   int idx;
    unsigned int size = 10;
    if (DistributedAllocator::world_rank == 0) {
        head = DistributedAllocator::alloc(size);
        MPI_Barrier(MPI_COMM_WORLD);
        DistributedAllocator::free(head);
        MPI_Barrier(MPI_COMM_WORLD);
        for (unsigned int i = 0; i < size; i++)
        {
            head = DistributedAllocator::alloc();
            std::cout << head << std::endl;
        }
    }
    MPI_Barrier(MPI_COMM_WORLD);
    DistributedAllocator::close();
    return 0;
}
