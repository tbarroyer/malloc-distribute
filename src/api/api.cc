# include <iostream>
# include <climits>
# include <thread>
# include <unistd.h>

# include "api.hh"

namespace api {

    // Initialize static variables
    int DistributedAllocator::world_size = 0;
    int DistributedAllocator::world_rank = 0;
    unsigned int DistributedAllocator::max_id = -1;

    std::map<unsigned int, int>* DistributedAllocator::collection = new std::map<unsigned int, int>();

    void DistributedAllocator::loop() {
        MPI_Request request;
        unsigned int data = 0;
        while(1) {
            for (int i = 0; i < world_size; ++i) {
                if (i == world_rank)
                    continue;
                MPI_Irecv(&data, 1, MPI_UNSIGNED, i, 0, MPI_COMM_WORLD, &request);
                std::cout << data << "\n";
            }
        }
    }

    void DistributedAllocator::init() {
        MPI_Init(NULL, NULL);
        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        max_id = world_rank * (UINT_MAX / world_size);

        std::thread async(loop);
        async.detach();
    }

    void DistributedAllocator::close() {
        MPI_Barrier(MPI_COMM_WORLD);
        MPI_Finalize();
        delete collection;
    }

    unsigned int DistributedAllocator::alloc() {
        std::cout << "Process " << world_rank << " is asking for memory" << std::endl;
        std::cout << "ID " << max_id << " given" << std::endl;

        // allocate memory
        (*collection)[max_id] = 0;

        return max_id++;
    }

    int DistributedAllocator::read(unsigned int id) {
        std::cout << "Process " << world_rank << " want to read" << std::endl;
        MPI_Request request;

        int process_id = id / (UINT_MAX / world_size);

        if (process_id == world_rank)
        {
            std::cout << "Process " << process_id
                      << " gave " << world_rank << " value " << (*collection)[id]
                      << std::endl;

            return (*collection)[id];
        }

        MPI_Isend(&id, 1, MPI_UNSIGNED, process_id, 0, MPI_COMM_WORLD, &request);

        int out;
        MPI_Recv(&out, 1, MPI_INT, process_id, 0, MPI_COMM_WORLD, MPI_STATUS_IGNORE);

        return out;
    }
}
