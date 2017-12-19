# include <iostream>
# include <unistd.h>
# include <climits>

# include <string>

# include "api/api.hh"

# define MAX_INT 50

using namespace api;

int main() {
    std::string gre = "\033[1;32m";
    std::string red = "\033[1;31m";
    std::string end = "\033[0m";

    DistributedAllocator::init();

    int world_rank = DistributedAllocator::world_rank;
    int world_size = DistributedAllocator::world_size;

    if (world_rank == 0) {
        std::cout << gre << "======= TEST SUITE =======" << end << "\n";
    }

    MPI_Barrier(MPI_COMM_WORLD);

    // TESTING ALLOCATIONS

    for (int i = 0; i < world_size; ++i)
    {
        if (i == world_rank)
        {
            bool ok = true;

            int start = world_rank * (MAX_INT / world_size);

            for (int j = start; j < start + 10; ++j)
            {
                int out = DistributedAllocator::alloc();
                int exp = j;

                if (out != exp) {
                    std::cout << "Allocation from " << world_rank << ": " << red << "FAILED" << end;
                    std::cout << "\tExpected " << exp << " but got " << out << "\n";
                    ok = false;
                }
            }

            for (int j = 0; j < 50; ++j)
            {
                if (j == start)
                    j += 10;

                int out = DistributedAllocator::alloc();
                int exp = j;

                if (out != exp) {
                    std::cout << "Allocation from " << world_rank << ": " << red << "FAILED" << end;
                    std::cout << "\tExpected " << exp << " but got " << out << "\n";
                    ok = false;
                }
                else if (ok && j == 49)
                    std::cout << "50 allocations from " << world_rank << ": " << gre << "PASSED" << end << "\n";
            }

            for (int j = 0; j < 50; ++j)
                DistributedAllocator::free(j);

            std::cout << "50 free from " << world_rank << ": " << gre << "PASSED" << end << "\n";
        }

        MPI_Barrier(MPI_COMM_WORLD);
    }


    DistributedAllocator::close();
    return 0;
}
