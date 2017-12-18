# include <iostream>
# include <climits>
# include <thread>
# include <unistd.h>

# include "api.hh"
//# define MAX_INT 10
# define MAX_INT INT_MAX
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
    std::map<int, std::pair<int, int>>* DistributedAllocator::collection = new std::map<int, std::pair<int, int>>();

    std::thread DistributedAllocator::re = std::thread();
    std::thread DistributedAllocator::se = std::thread();

    std::queue<std::pair<int, int>>* DistributedAllocator::send_value = new std::queue<std::pair<int, int>>();
    std::queue<std::pair<int, int>>* DistributedAllocator::send_free = new std::queue<std::pair<int, int>>();
    std::queue<std::pair<int, int>>* DistributedAllocator::ret_free = new std::queue<std::pair<int, int>>();
    std::queue<std::pair<int, int>>* DistributedAllocator::send_key =   new std::queue<std::pair<int, int>>();
    std::queue<std::pair<int, int>>* DistributedAllocator::send_alloc_req =  new std::queue<std::pair<int, int>>();
    std::queue<std::pair<int, int>>* DistributedAllocator::send_alloc_resp = new std::queue<std::pair<int, int>>();

    std::queue<std::pair<int, std::pair<int, int>>>* DistributedAllocator::send_key_write =
        new std::queue<std::pair<int, std::pair<int, int>>>();

    std::mutex DistributedAllocator::m;
    std::mutex DistributedAllocator::m_get;

    std::condition_variable DistributedAllocator::cv;
    std::condition_variable DistributedAllocator::cv_get;

    std::vector<int>* DistributedAllocator::free_disp = new std::vector<int>();

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
                send_value->push(std::make_pair(status.MPI_SOURCE, (*collection)[buf[0]].second));
                std::cout << "Receive thread break" << std::endl;
                cv.notify_one();
                break;
            }

            // Someone wants to access my memory
            if (status.MPI_TAG == 99) {
                if (buf[0] != -1) {
                    send_value->push(std::make_pair(status.MPI_SOURCE, (*collection)[buf[0]].second));
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

            else if (status.MPI_TAG == 33) {
                buff_value = buf[0];
                get_ready = true;
                cv_get.notify_one();
            }

            else if (status.MPI_TAG == 11) {
                free_disp->push_back(buf[0]);
                ret_free->push(std::make_pair(status.MPI_SOURCE, 1));
                cv.notify_one();
            }

            // Someone want to change my memory
            else if (status.MPI_TAG == 88) {
                (*collection)[buf[0]] = std::make_pair((*collection)[buf[0]].first, buf[1]);
                send_key->push(std::make_pair(status.MPI_SOURCE, -1));
                cv.notify_one();
            }
            // Someone want to alloc my memory
            else if (status.MPI_TAG == 66) {
                if (cur_id < max_id)
                {
                    (*collection)[cur_id] = std::make_pair(-1, 42);
                    send_alloc_resp->push(std::make_pair(status.MPI_SOURCE, cur_id));
                    cur_id++;
                }
                else
                    send_alloc_resp->push(std::make_pair(status.MPI_SOURCE, -1));
                cv.notify_one();
            }
            // Someone returns me the id of the alloc I asked for
            else if (status.MPI_TAG == 22) {
                buff_value = buf[0];
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
            cv.wait(lk, []{return !send_value->empty() || !send_key->empty() ||!send_key_write->empty() || !send_alloc_req->empty() || !send_alloc_resp->empty()
                               || !send_free->empty() || !ret_free->empty();});

            std::pair<int, int> pair = std::make_pair(-1, -1);
            std::pair<int, int> pair_key = std::make_pair(-1, 0);
            std::pair<int, int> pair_alloc_req = std::make_pair(-1, 0);
            std::pair<int, int> pair_alloc_resp = std::make_pair(-1, 0);
            std::pair<int, std::pair<int, int>> pair_key_write = std::make_pair(-1, std::make_pair(-1, 0));

            // Send all values
            while (!send_value->empty())
            {
                pair = send_value->front();

                if (pair.first == world_rank)
                {
                    break;
                }

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

            while (!send_alloc_req->empty())
            {
                pair_alloc_req = send_alloc_req->front();
                MPI_Isend(&(pair_alloc_req.second), 1, MPI_INT, pair_alloc_req.first, 66, MPI_COMM_WORLD, &request);
                send_alloc_req->pop();
            }

            while (!send_alloc_resp->empty())
            {
                pair_alloc_resp = send_alloc_resp->front();
                MPI_Isend(&(pair_alloc_resp.second), 1, MPI_INT, pair_alloc_resp.first, 22, MPI_COMM_WORLD, &request);
                send_alloc_resp->pop();
            }

            while (!send_free->empty())
            {
                pair_key = send_free->front();
                MPI_Isend(&(pair_key.second), 1, MPI_INT, pair_key.first, 11, MPI_COMM_WORLD, &request);
                send_free->pop();
            }

            while (!ret_free->empty())
            {
                pair_key = ret_free->front();
                MPI_Isend(&(pair_key.second), 1, MPI_INT, pair_key.first, 33, MPI_COMM_WORLD, &request);
                ret_free->pop();
            }

            if (pair.first == world_rank)
            {
                break;
            }
        }
    }

    void DistributedAllocator::init() {
        int provided;
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);

        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);
        max_id = (world_rank + 1) * (MAX_INT / world_size);
        cur_id = world_rank * (MAX_INT / world_size);

        std::cout << "Process " << world_rank << " initialized." << std::endl;

        re = std::thread(loop_re);
        se = std::thread(loop_se);
        MPI_Barrier(MPI_COMM_WORLD);
    }

    void DistributedAllocator::close() {
        MPI_Barrier(MPI_COMM_WORLD);

        int toto;

        MPI_Request request;
        MPI_Isend(&toto, 1, MPI_INT, world_rank, 0, MPI_COMM_WORLD, &request);

        re.native_handle();
        se.native_handle();
        re.join();
        se.join();

        delete collection;
        delete send_value;
        delete send_key;

        MPI_Barrier(MPI_COMM_WORLD);

        MPI_Finalize();
    }


    void DistributedAllocator::free(int id) {
        std::cout << "Process " << world_rank << " want to free" << std::endl;
        int process_id = id / (MAX_INT / world_size);

        if (process_id == world_rank) {
            free_disp->push_back(id);
            //BEGIN HAMZA
            //free all the nexts ids
            int next_id =(*collection)[id].first;
            while((next_id != -1) && ((next_id/ (MAX_INT / world_size)) == world_rank) )
            {
              free_disp->push_back(next_id);
              next_id =(*collection)[next_id].first;
            }
            //if exited the loop because next_id is not in the current process
            if(next_id != -1)
            {
                send_free->push(std::make_pair(process_id, id));
                cv.notify_one();

                std::unique_lock<std::mutex> lk(m);
                cv_get.wait(lk, []{return get_ready;});
                get_ready = false;
            }
            //END HAMZA
        }
        else {
            send_free->push(std::make_pair(process_id, id));
            cv.notify_one();

            std::unique_lock<std::mutex> lk(m);
            cv_get.wait(lk, []{return get_ready;});
            get_ready = false;
        }
    }

    int DistributedAllocator::alloc(unsigned int size) {
        std::cout << "Process " << world_rank << " is asking for memory of size " << size << std::endl;
        int ret_idx = alloc();
        int first_idx = ret_idx;
        if (first_idx == -1)
            return first_idx;
        int second_idx;
        for (unsigned int i = 1; i < size; i++)
        {
            second_idx = alloc();
            if (second_idx == -1)
                break;
            (*collection)[first_idx].first = second_idx;
            std::cout << "Key " << first_idx << " with value "
                << (*collection)[first_idx].second << " has next id "
                << (*collection)[first_idx].first << std::endl;
            first_idx = second_idx;
        }
        return ret_idx;
    }

    int DistributedAllocator::alloc() {
        std::cout << "Process " << world_rank << " is asking for memory" << std::endl;
        int alloc_idx = -1;
        // allocate memory
        if (cur_id < max_id)
        {
            if (free_disp->empty()) {
                std::cout << "ID " << cur_id << " given" << std::endl;
                (*collection)[cur_id] = std::make_pair(-1, 42);
                alloc_idx = cur_id;
                cur_id++;
            }
            else {
                alloc_idx = free_disp->back();
                std::cout << "ID " << alloc_idx << " given" << std::endl;
                (*collection)[alloc_idx] = std::make_pair(-1, 42);
                free_disp->pop_back();
            }
        }
        else
        {
            // send alloc_queue
            for (int i = 0; i < world_size; i++)
            {
                if (i == world_rank)
                    continue;

                send_alloc_req->push(std::make_pair(i, i));
                cv.notify_one();

                // Wait until my received thread get the result
                std::unique_lock<std::mutex> lk(m);
                cv_get.wait(lk, []{return get_ready;});

                int out = buff_value;
                get_ready = false;

                std::cout << "Process " << world_rank
                    << " asking Process " << i << " to allocate memory"
                    << std::endl;
                if (!(out == -1))
                {
                    std::cout << "Answer of Process " << i
                        << " to allocate memory asked from Process "
                        << world_rank << " is yes and ID is " << buff_value << std::endl;
                    alloc_idx = buff_value;
                    break;
                }
            }
        }
        if (alloc_idx == -1)
            std::cout << "Process " << world_rank
                << " tried to allocate memory asked but no space is available." << std::endl;

        return alloc_idx;
    }

    int DistributedAllocator::read(int id) {
        std::cout << "Process " << world_rank << " want to read" << std::endl;

        int process_id = id / (MAX_INT / world_size);

        if (process_id == world_rank)
        {
            std::cout << "Process " << process_id
                << " gave " << world_rank << " value " << (*collection)[id].second
                << std::endl;

            return (*collection)[id].second;
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

    int DistributedAllocator::next(int id)
    {
        return (*collection)[id].first;
    }

    bool DistributedAllocator::write(int id, int value) {
        std::cout << "Process " << world_rank << " want to write" << std::endl;

        int process_id = id / (MAX_INT / world_size);

        if (process_id == world_rank) {
            std::cout << "Process " << process_id
                << " write " << world_rank << " value " << value
                << std::endl;

            //(*collection)[id] = std::make_pair(-1, value);
            (*collection)[id].second = value;

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
    //BEGIN HAMZA

      bool DistributedAllocator::write(int id, int* vals, unsigned int size)
      {
        bool ret = true ;
        unsigned int i = 0 ;
        int next_id = id ;
        while ((i < size) && (next_id != -1))
        {
          ret = ret && write(next_id, *(vals + i));
          next_id = next(next_id);
          i++;
        }
        if ((i != size) && (next_id == -1))
        {
          std::cout  << "Process " << world_rank
             << " not enought memory allocated to write all the data "
             << std::endl;
          return false;
        }
        return ret;

      }

    //END HAMZA
}
