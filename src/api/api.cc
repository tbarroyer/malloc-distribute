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

    std::thread DistributedAllocator::re = std::thread();
    std::thread DistributedAllocator::se = std::thread();
        
    std::queue<std::pair<int, int>>* DistributedAllocator::send_value = new std::queue<std::pair<int, int>>();
//  std::queue<int>* send_key = new std::queue<int>();
        
    std::mutex DistributedAllocator::m;
    std::condition_variable DistributedAllocator::cv;

    void DistributedAllocator::loop_re() {
        unsigned int buf;
        MPI_Status status;
        while (1) {
            MPI_Recv(&buf, 1, MPI_UNSIGNED, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_SOURCE == world_rank)
            {
                send_value->push(std::make_pair(status.MPI_SOURCE, (*collection)[buf]));
                cv.notify_one();
                break;
            }
            send_value->push(std::make_pair(status.MPI_SOURCE, (*collection)[buf]));
            cv.notify_one();
        }
    }

    void DistributedAllocator::loop_se() {
        MPI_Request request;
        while (1) {
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, []{return !send_value->empty();});
            std::pair<int, int> pair;
            while (!send_value->empty())
            {
                pair = send_value->front();

                if (pair.first == world_rank)
                    break;

                MPI_Isend(&(pair.second), 1, MPI_UNSIGNED, pair.first, 77, MPI_COMM_WORLD, &request);
                send_value->pop();
            }
            if (pair.first == world_rank)
                break;
        }
    }

    void DistributedAllocator::init() {
        int provided ;
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);

        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        max_id = world_rank * (UINT_MAX / world_size);

        re = std::thread(loop_re);
        se = std::thread(loop_se);
    }

    void DistributedAllocator::close() {
        MPI_Barrier(MPI_COMM_WORLD);


        unsigned int toto;

        MPI_Request request;
        MPI_Isend(&toto, 1, MPI_UNSIGNED, world_rank, 0, MPI_COMM_WORLD, &request);

        re.join();
        se.join();

        delete collection;
        delete send_value;
//        delete send_key;
        
        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Finalize();
    }

    unsigned int DistributedAllocator::alloc() {
        std::cout << "Process " << world_rank << " is asking for memory" << std::endl;
        std::cout << "ID " << max_id << " given" << std::endl;

        // allocate memory
        (*collection)[max_id] = 42;

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

        int out = -1;
        MPI_Recv(&out, 1, MPI_INT, process_id, 77, MPI_COMM_WORLD, MPI_STATUS_IGNORE);
            
        std::cout << "Process " << process_id
                  << " gave " << world_rank << " value " << out
                  << std::endl;

        return out;
    }
}
