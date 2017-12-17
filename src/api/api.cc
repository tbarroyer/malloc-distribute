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

    int DistributedAllocator::buff_value = 0;
    bool DistributedAllocator::get_ready = false;
    
    std::map<unsigned int, int>* DistributedAllocator::collection = new std::map<unsigned int, int>();

    std::thread DistributedAllocator::re = std::thread();
    std::thread DistributedAllocator::se = std::thread();
        
    std::queue<std::pair<int, int>>* DistributedAllocator::send_value = new std::queue<std::pair<int, int>>();
    std::queue<std::pair<int, unsigned int>>* DistributedAllocator::send_key =
        new std::queue<std::pair<int, unsigned int>>();
        
    std::mutex DistributedAllocator::m;
    std::mutex DistributedAllocator::m_get;

    std::condition_variable DistributedAllocator::cv;
    std::condition_variable DistributedAllocator::cv_get;

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

            if (status.MPI_TAG == 99) {
                send_value->push(std::make_pair(status.MPI_SOURCE, (*collection)[buf]));
                cv.notify_one();
            }
            else if (status.MPI_TAG == 77) {
                buff_value = buf;
                get_ready = true;
                cv_get.notify_one();
            }
        }
    }

    void DistributedAllocator::loop_se() {
        MPI_Request request;
        while (1) {

            // Wait until a send is needed
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, []{return !send_value->empty() || !send_key->empty();});

            std::pair<int, int> pair = std::make_pair(-1, -1);
            std::pair<int, unsigned int> pair_key = std::make_pair(-1, 0);

            // Send all values
            while (!send_value->empty())
            {
                pair = send_value->front();

                if (pair.first == world_rank)
                    break;
                
                MPI_Isend(&(pair.second), 1, MPI_INT, pair.first, 77, MPI_COMM_WORLD, &request);
                send_value->pop();
            }

            // Send all keys
            while (!send_key->empty())
            {
                pair_key = send_key->front();

                MPI_Isend(&(pair_key.second), 1, MPI_UNSIGNED, pair_key.first, 99, MPI_COMM_WORLD, &request);
                send_key->pop();
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
        delete send_key;
        
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

        int process_id = id / (UINT_MAX / world_size);

        if (process_id == world_rank)
        {
            std::cout << "Process " << process_id
                      << " gave " << world_rank << " value " << (*collection)[id]
                      << std::endl;

            return (*collection)[id];
        }

        send_key->push(std::make_pair(process_id, id));
        cv.notify_one();

        int out = -1;
            
        std::unique_lock<std::mutex> lk(m);
        cv_get.wait(lk, []{return get_ready;});

        out = buff_value;
        get_ready = false;

        std::cout << "Process " << process_id
                  << " gave " << world_rank << " value " << out
                  << std::endl;

        return out;
    }
}
