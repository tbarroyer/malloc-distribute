# include <iostream>
# include <climits>
# include <thread>
# include <unistd.h>

# include "api.hh"

namespace api {

    // Initialize static variables
    // ===========================
    // ===========================
    // ===========================
    int DistributedAllocator::world_size = 0;
    int DistributedAllocator::world_rank = 0;

    int DistributedAllocator::max_id = -1;
    int DistributedAllocator::cur_id = -1;

    int DistributedAllocator::buff_value = 0;
    bool DistributedAllocator::get_ready = false;
    std::map<int, int>* DistributedAllocator::collection = new std::map<int, int>();

    std::thread DistributedAllocator::re = std::thread();
    std::thread DistributedAllocator::se = std::thread();

    std::queue<std::pair<int, int>>* DistributedAllocator::send_value = new std::queue<std::pair<int, int>>();
    std::queue<std::pair<int, int>>* DistributedAllocator::send_key =   new std::queue<std::pair<int, int>>();

    std::queue<std::pair<int, std::pair<int, int>>>* DistributedAllocator::send_key_write =
        new std::queue<std::pair<int, std::pair<int, int>>>();

    std::mutex DistributedAllocator::m;
    std::mutex DistributedAllocator::m_get;

    std::condition_variable DistributedAllocator::cv;
    std::condition_variable DistributedAllocator::cv_get;


    // Member definitions ========
    // ===========================
    // ===========================
    // ===========================

    void DistributedAllocator::loop_re() {
        int* buf = (int*)malloc(2 * sizeof (int));
        MPI_Status status;
        while (1) {

            // Wait until it received something
            MPI_Recv(buf, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);
            if (status.MPI_SOURCE == world_rank) {
                send_value->push(std::make_pair(status.MPI_SOURCE, (*collection)[buf[0]]));
                cv.notify_one();
                break;
            }

            // Someone wants to access my memory
            if (status.MPI_TAG == 99) {
                if (buf[0] != -1) {
                    send_value->push(std::make_pair(status.MPI_SOURCE, (*collection)[buf[0]]));
                    cv.notify_one();
                }
                else if (buf[0] == -1) {
                    cv_get.notify_one();
                    get_ready = true;
                    buff_value = 1;
                    cv_get.notify_one();
                }
            }

            // Someone returns me its memory
            else if (status.MPI_TAG == 77) {
                buff_value = buf[0];
                get_ready = true;
                cv_get.notify_one();
            }

            // Someone want to change my memory
            else if (status.MPI_TAG == 88) {
                (*collection)[buf[0]] = buf[1];
                send_key->push(std::make_pair(status.MPI_SOURCE, -1));
                cv.notify_one();
            }
        }
    }

    void DistributedAllocator::loop_se() {
        MPI_Request request;
        while (1) {

            // Wait until a send is needed
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, []{return !send_value->empty() || !send_key->empty() ||!send_key_write->empty();});

            std::pair<int, int> pair = std::make_pair(-1, -1);
            std::pair<int, int> pair_key = std::make_pair(-1, 0);
            std::pair<int, std::pair<int, int>> pair_key_write = std::make_pair(-1, std::make_pair(-1, 0));

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

                MPI_Isend(&(pair_key.second), 1, MPI_INT, pair_key.first, 99, MPI_COMM_WORLD, &request);
                send_key->pop();
            }

            // Send all keys/values
            while (!send_key_write->empty())
            {
                pair_key_write = send_key_write->front();

                // FIXME FREE
                int* sent = (int*)malloc(2 * sizeof (int));
                sent[0] = pair_key_write.second.first;
                sent[1] = pair_key_write.second.second;

                MPI_Isend(sent, 2, MPI_INT, pair_key_write.first, 88, MPI_COMM_WORLD, &request);

                send_key_write->pop();
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
        max_id = (world_rank + 1) * (INT_MAX / world_size);
        cur_id = world_rank * (INT_MAX / world_size);

        re = std::thread(loop_re);
        se = std::thread(loop_se);
    }

    void DistributedAllocator::close() {
        MPI_Barrier(MPI_COMM_WORLD);

        int toto;

        MPI_Request request;
        MPI_Isend(&toto, 1, MPI_INT, world_rank, 0, MPI_COMM_WORLD, &request);

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
        std::cout << "ID " << cur_id << " given" << std::endl;

        // allocate memory
        (*collection)[cur_id] = 42;

        return cur_id++;
    }

    int DistributedAllocator::read(int id) {
        std::cout << "Process " << world_rank << " want to read" << std::endl;

        int process_id = id / (INT_MAX / world_size);

        if (process_id == world_rank)
        {
            std::cout << "Process " << process_id
                      << " gave " << world_rank << " value " << (*collection)[id]
                      << std::endl;

            return (*collection)[id];
        }

        send_key->push(std::make_pair(process_id, id));
        cv.notify_one();

        // Wait until my received thread get the result
        std::unique_lock<std::mutex> lk(m);
        cv_get.wait(lk, []{return get_ready;});

        int out = buff_value;
        get_ready = false;

        std::cout << "Process " << process_id
                  << " gave " << world_rank << " value " << out
                  << std::endl;

        return out;
    }

    bool DistributedAllocator::write(int id, int value) {
        std::cout << "Process " << world_rank << " want to write" << std::endl;

        int process_id = id / (INT_MAX / world_size);

        if (process_id == world_rank) {
            std::cout << "Process " << process_id
                      << " write " << world_rank << " value " << value
                      << std::endl;

            (*collection)[id] = value;

            return true;
        }

        send_key_write->push(std::make_pair(process_id, std::make_pair(id, value)));
        cv.notify_one();

        // Wait until my received thread get the result
        std::unique_lock<std::mutex> lk(m);
        cv_get.wait(lk, []{return get_ready;});

        bool out = (bool)buff_value;
        get_ready = false;

        std::cout << "Process " << world_rank
                  << " change process " << process_id << " value "
                  << std::endl;

        return out;
    }
}
