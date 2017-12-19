# include <iostream>
# include <climits>
# include <thread>
# include <unistd.h>

# include "api.hh"
# define MAX_INT 10
//# define MAX_INT INT_MAX
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

    std::queue<Message>* DistributedAllocator::send_queue = new std::queue<Message>();

    std::mutex DistributedAllocator::m;
    std::mutex DistributedAllocator::m_get;

    std::condition_variable DistributedAllocator::cv;
    std::condition_variable DistributedAllocator::cv_get;

    std::queue<int>* DistributedAllocator::free_disp = new std::queue<int>();

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
            if (status.MPI_SOURCE == world_rank && status.MPI_TAG == 4) {

//                std::cout << "Receive thread break" << std::endl;
                cv.notify_one();
                break;
            }

            // Someone wants to access my memory
            if (status.MPI_TAG == 99) {
                Message m = {status.MPI_SOURCE, 77, {(*collection)[buf[0]].second, -1}};
                send_queue->push(m);

                cv.notify_one();
            }

            else if (status.MPI_TAG == 77 || status.MPI_TAG == 33 || status.MPI_TAG == 22 ||
                     status.MPI_TAG == 76 || status.MPI_TAG == 45 ) {
                buff_value = buf[0];
                get_ready = true;
                cv_get.notify_one();
            }

            else if (status.MPI_TAG == 11) {
                free(buf[0]);
                Message m = {status.MPI_SOURCE, 33, {1, -1}};
                send_queue->push(m);

                cv.notify_one();
            }

            // Someone want to change my memory
            else if (status.MPI_TAG == 88) {
                (*collection)[buf[0]] = std::make_pair((*collection)[buf[0]].first, buf[1]);
                Message m = {status.MPI_SOURCE, 76, {-1, -1}};
                send_queue->push(m);

                cv.notify_one();
            }
            // Someone want to alloc my memory
            else if (status.MPI_TAG == 66) {
                if (cur_id < max_id)
                {
                    (*collection)[cur_id] = std::make_pair(-1, 42);

                    Message m = {status.MPI_SOURCE, 22, {cur_id, -1}};
                    send_queue->push(m);

                    cur_id++;
                }
                else {
                    Message m = {status.MPI_SOURCE, 22, {-1, -1}};
                    send_queue->push(m);
                }

                cv.notify_one();
            }
            // Someone want to alloc rest collection
            else if (status.MPI_TAG == 44) {
                int idx = alloc(buf[0]);
                Message m = {status.MPI_SOURCE, 45, {idx, -1}};
                send_queue->push(m);
                cv.notify_one();
            }
        }
    }

    void DistributedAllocator::loop_se() {
        MPI_Request request;
        while (1) {

            // Wait until a send is needed
            std::unique_lock<std::mutex> lk(m);
            cv.wait(lk, []{return !send_queue->empty();});

            Message msg;

            // Send all values
            while (!send_queue->empty())
            {
                msg = send_queue->front();

                MPI_Isend(&(msg.data), 2, MPI_INT, msg.process_id, msg.tag, MPI_COMM_WORLD, &request);
                send_queue->pop();

                if (msg.process_id == world_rank && msg.tag == 4)
                {
//                    std::cout << "Send thread break" << std::endl;
                    break;
                }
            }

            if (msg.process_id == world_rank && msg.tag == 4)
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

//        std::cout << "Process " << world_rank << " initialized." << std::endl;

        re = std::thread(loop_re);
        se = std::thread(loop_se);

        MPI_Barrier(MPI_COMM_WORLD);
    }

    void DistributedAllocator::close() {
//        std::cout << "WAIT QUIT " << world_rank << "\n";

        MPI_Barrier(MPI_COMM_WORLD);

        Message msg = {world_rank, 4, {-1, -1}};
        send_queue->push(msg);
        cv.notify_one();

        re.join();
        se.join();

        delete collection;
        delete send_queue;

        MPI_Barrier(MPI_COMM_WORLD);

 //       std::cout << "QUIT " << world_rank << "\n";

        MPI_Finalize();
    }


    void DistributedAllocator::free(int id) {
//        std::cout << "Process " << world_rank << " want to free id " << id << std::endl;
        int process_id = id / (MAX_INT / world_size);;
        //int next_id =(*collection)[id].first;
        while (id != -1 && process_id == world_rank)
        {
//            std::cout << "Process " << world_rank << " freed id " << id << " on his memory" << std::endl;

            free_disp->push(id);
            id = (*collection)[id].first;
            if (id != -1)
                (*collection)[id - 1].first = -1;
            process_id = id / (MAX_INT / world_size);
//            std::cout << "Next id: " << id << std::endl;
        }

        if (id != -1)
        {
            Message msg = {process_id, 11, {id, -1}};
            send_queue->push(msg);
            cv.notify_one();

            std::unique_lock<std::mutex> lk(m);
            cv_get.wait(lk, []{return get_ready;});
            get_ready = false;
        }
    }


    int DistributedAllocator::alloc(unsigned int size) {
//        std::cout << "Process " << world_rank << " is asking for memory of size " << size << std::endl;
        int ret_idx = alloc();
        int first_idx = ret_idx;
        if (first_idx == -1)
            return first_idx;
        int second_idx;
        unsigned int i = 1;
        int process_id = cur_id / (MAX_INT / world_size);
        while ((cur_id < max_id) && (i < size) && (world_rank == process_id))
        {
            second_idx = alloc();
            if (second_idx == -1)
                break;
            (*collection)[first_idx].first = second_idx;
//            std::cout << "Key " << first_idx << " with value "
//                << (*collection)[first_idx].second << " has next id "
//                << (*collection)[first_idx].first << std::endl;
            first_idx = second_idx;
            process_id = cur_id / (MAX_INT / world_size);
            i++;
        }
        if ((cur_id == max_id) && (i < size) )
        {
          int nsize = size - i;
          Message message = {process_id, 44, {nsize, -1}};

          send_queue->push(message);

          cv.notify_one();

        // Wait until my received thread get the result
          std::unique_lock<std::mutex> lk(m);
          cv_get.wait(lk, []{return get_ready;});

          int out = buff_value;
          get_ready = false;
//          std::cout <<"Tag Message 44 " << first_idx <<" has next out "
//          << out  <<" from procees : " << process_id << std::endl ;
            (*collection)[first_idx].first =  out  ;
          }
        return ret_idx;
    }

    int DistributedAllocator::alloc() {
//        std::cout << "Process " << world_rank << " is asking for memory" << std::endl;
        int alloc_idx = -1;
        // allocate memory
        if (!free_disp->empty()) {
            alloc_idx = free_disp->front();
            //std::cout << "ID " << alloc_idx << " given" << std::endl;
            (*collection)[alloc_idx] = std::make_pair(-1, 42);
            free_disp->pop();
            return alloc_idx;
        }
        if (cur_id < max_id)
        {
            //std::cout << "ID " << cur_id << " given" << std::endl;
            (*collection)[cur_id] = std::make_pair(-1, 42);
            alloc_idx = cur_id;
            cur_id++;
        }
        else
        {
            // send alloc_queue
            for (int i = 0; i < world_size; i++)
            {
                if (i == world_rank)
                    continue;

                Message message = {i, 66, {i, -1}};
                send_queue->push(message);

                cv.notify_one();

                // Wait until my received thread get the result
                std::unique_lock<std::mutex> lk(m);
                cv_get.wait(lk, []{return get_ready;});

                int out = buff_value;
                get_ready = false;

//                std::cout << "Process " << world_rank
//                    << " asking Process " << i << " to allocate memory"
//                  << std::endl;
                if (!(out == -1))
                {
//                    std::cout << "Answer of Process " << i
//                        << " to allocate memory asked from Process "
//                        << world_rank << " is yes and ID is " << buff_value << std::endl;
                    alloc_idx = buff_value;
                    break;
                }
            }
        }
//        if (alloc_idx == -1)
//            std::cout << "Process " << world_rank
//                << " tried to allocate memory asked but no space is available." << std::endl;

        return alloc_idx;
    }

    int DistributedAllocator::read(int id) {
//        std::cout << "Process " << world_rank << " want to read" << std::endl;

        int process_id = id / (MAX_INT / world_size);

        if (process_id == world_rank)
        {
//            std::cout << "Process " << process_id
//                << " gave " << world_rank << " value " << (*collection)[id].second
//                << std::endl;

            return (*collection)[id].second;
        }

        Message msg = {process_id, 99, {id, -1}};
        send_queue->push(msg);

        cv.notify_one();

        // Wait until my received thread get the result
        std::unique_lock<std::mutex> lk(m);
        cv_get.wait(lk, []{return get_ready;});

        int out = buff_value;
        get_ready = false;

//        std::cout << "Process " << process_id
//            << " gave " << world_rank << " value " << out
//            << std::endl;

        return out;
    }

    int DistributedAllocator::next(int id)
    {
        return (*collection)[id].first;
    }

    bool DistributedAllocator::write(int id, int value) {
//        std::cout << "Process " << world_rank << " want to write" << std::endl;

        int process_id = id / (MAX_INT / world_size);

        if (process_id == world_rank) {
//            std::cout << "Process " << process_id
//                << " write " << world_rank << " value " << value
//                << std::endl;

            (*collection)[id].second = value;

            return true;
        }

        cv.notify_one();

        // Wait until my received thread get the result
        std::unique_lock<std::mutex> lk(m);
        cv_get.wait(lk, []{return get_ready;});

        bool out = (bool)buff_value;
        get_ready = false;

//        std::cout << "Process " << world_rank
//            << " change process " << process_id << " value "
//            << std::endl;

        return out;
    }

    bool DistributedAllocator::write(int id, int* vals, unsigned int size)
    {
        bool ret = true;
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
//            std::cout  << "Process " << world_rank
//                       << " not enought memory allocated to write all the data "
//                       << std::endl;
            return false;
        }
        return ret;
    }
}
