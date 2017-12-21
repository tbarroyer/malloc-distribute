# include <iostream>
# include <climits>
# include <thread>
# include <unistd.h>
# include <chrono>

# include "api.hh"
# define MAX_INT 320
//# define MAX_INT INT_MAX
namespace api {
    // Static variable initialisation
    bool DistributedAllocator::last = false;
    bool DistributedAllocator::demand = false;
    
    int DistributedAllocator::last_head = -1;  
    int DistributedAllocator::first_head = -1;  

    int DistributedAllocator::world_size = 0;
    int DistributedAllocator::world_rank = 0;

    int DistributedAllocator::max_id = -1;
    int DistributedAllocator::cur_id = -1;

    int DistributedAllocator::buff_value = 0;
    bool DistributedAllocator::get_ready = false;

    std::map<int, std::pair<int, int>>* DistributedAllocator::collection = 
      new std::map<int, std::pair<int, int>>();

    std::thread DistributedAllocator::re = std::thread();
    std::thread DistributedAllocator::se = std::thread();

    std::queue<Message>* DistributedAllocator::send_queue = new std::queue<Message>();

    std::mutex DistributedAllocator::m;
    std::mutex DistributedAllocator::m_get;

    std::condition_variable DistributedAllocator::cv;
    std::condition_variable DistributedAllocator::cv_get;

    std::queue<int>* DistributedAllocator::free_disp = new std::queue<int>();

    // Received loop for receive thrad
    void DistributedAllocator::loop_re()
    {
        // Received buffer
        int* buf = (int*)malloc(2 * sizeof (int));

        // Keep last sender when allocating array
        int id_pro = -1;

        MPI_Status status;

        while (1) {
            // Wait until it received something
            MPI_Recv(buf, 2, MPI_INT, MPI_ANY_SOURCE, MPI_ANY_TAG, MPI_COMM_WORLD, &status);

            // Used to close the thead
            if (status.MPI_SOURCE == world_rank && status.MPI_TAG == 4)
                // Exit the thread
                break;

            // Someone wants to access my memory
            if (status.MPI_TAG == 99)
            {
                // Returning memory on chan 77
                Message m = {status.MPI_SOURCE, 77, {(*collection)[buf[0]].second, -1}};
                send_queue->push(m);

                // Notify the sending thread
                cv.notify_one();
            }

            // Someone want to get the next element of an id in an array
            else if (status.MPI_TAG == 101)
            {
                // Sending him back the id of the next element
                Message m = {status.MPI_SOURCE, 77, {(*collection)[buf[0]].first, -1}};
                send_queue->push(m);

                // Notify the sending thread
                cv.notify_one();
            }

            // Someone return me a value
            else if (status.MPI_TAG == 77 || status.MPI_TAG == 33 || status.MPI_TAG == 22 ||
                     status.MPI_TAG == 76)
            {
                // Put the value in a shared variable (between threads of same process)
                buff_value = buf[0];

                // Notify the process
                get_ready = true;
                cv_get.notify_one();
            }

            // Someone want to free my memory
            else if (status.MPI_TAG == 11)
            {
                // Free memory
                free(buf[0]);

                // Send him back ok
                Message m = {status.MPI_SOURCE, 33, {1, -1}};
                send_queue->push(m);

                // Notify the sending thread
                cv.notify_one();
            }

            // Someone want to write on my memory
            else if (status.MPI_TAG == 88)
            {
                // Write in memory
                (*collection)[buf[0]] = std::make_pair((*collection)[buf[0]].first, buf[1]);

                // Send him back ok
                Message m = {status.MPI_SOURCE, 76, {-1, -1}};
                send_queue->push(m);

                // Notify the sending process
                cv.notify_one();
            }

            // Someone want to alloc my memory
            else if (status.MPI_TAG == 66)
            {
                // If we have some space left
                if (cur_id < max_id || !free_disp->empty())
                {
                    int out = alloc();

                    // Return id
                    Message m = {status.MPI_SOURCE, 22, {out, -1}};
                    send_queue->push(m);
                }

                // If I don't have some space left, 
                else
                {
                    // Return KO
                    Message m = {status.MPI_SOURCE, 22, {-1, -1}};
                    send_queue->push(m);
                }

                // Notify sending thread
                cv.notify_one();
            }

            // Someone want to alloc rest collection
            else if (status.MPI_TAG == 44)
            {
                // Launching asyncronous alloc
                async_alloc(buf[0]);
                // Save request id
                id_pro = status.MPI_SOURCE;
            }

            // Someone return me allocation result
            else if (status.MPI_TAG == 45)
            {
                buff_value = buf[0];

                // If we are not the request
                if (!demand)
                {
                    // Send back the value
                    Message m = {id_pro, 45, {last_head, -1}};
                    send_queue->push(m);
                    cv.notify_one();
                }
                // If we are the request
                else
                {
                    // Notify process we got the answer
                    get_ready = true;
                    cv_get.notify_one();
                }
                
                // Linking the two parts
                (*collection)[first_head].first = buff_value;
                
                // Reset state
                first_head = -1;
                last_head = -1;
            }

            // If async all finished
            if (last)
            {
                // send back the result
                Message m = {status.MPI_SOURCE, 45, {last_head, -1}};
                send_queue->push(m);
                cv.notify_one();

                // Reset state
                last_head = -1;
                last = false;
            }
        }
    }

    // Send loop for sending thread
    void DistributedAllocator::loop_se()
    {
        MPI_Request request;

        while (1)
        {
            // Wait until a send is needed
            std::unique_lock<std::mutex> lk(m);
            cv.wait_for(lk, std::chrono::milliseconds(500),  []{return !send_queue->empty();});

            Message msg;

            // Send all values
            while (!send_queue->empty())
            {
                msg = send_queue->front();

                MPI_Isend(&(msg.data), 2, MPI_INT, msg.process_id, msg.tag, MPI_COMM_WORLD, &request);
                send_queue->pop();

                // The close function has been called, getting out the loop
                if (msg.process_id == world_rank && msg.tag == 4)
                    break;
            }

            // The close function has been called, getting out of the loop
            if (msg.process_id == world_rank && msg.tag == 4)
                break;
        }
    }

    void DistributedAllocator::init()
    {
        int provided;
        MPI_Init_thread(NULL, NULL, MPI_THREAD_MULTIPLE, &provided);

        MPI_Comm_size(MPI_COMM_WORLD, &world_size);
        MPI_Comm_rank(MPI_COMM_WORLD, &world_rank);

        // Init first id and max id
        max_id = (world_rank + 1) * (MAX_INT / world_size);
        cur_id = world_rank * (MAX_INT / world_size);

        // Launching receiving thread
        re = std::thread(loop_re);
        // Launching sending thread
        se = std::thread(loop_se);

        // Make sure every thread had been launch
        MPI_Barrier(MPI_COMM_WORLD);
    }

    void DistributedAllocator::close()
    {
        // Make sure all process have finished before closing threads
        MPI_Barrier(MPI_COMM_WORLD);

        // Send a message to close threads
        Message msg = {world_rank, 4, {-1, -1}};
        send_queue->push(msg);
        cv.notify_one();

        // Waiting for thread to quit
        re.join();
        se.join();
        
        delete collection;
        delete send_queue;

        MPI_Barrier(MPI_COMM_WORLD);
        
        MPI_Finalize();
    }


    void DistributedAllocator::free(int id)
    {
        int process_id = id / (MAX_INT / world_size);

        // For each element in array that are in the process, free
        while (id != -1 && process_id == world_rank)
        {
            free_disp->push(id);
            id = (*collection)[id].first;

            if (id != -1)
                (*collection)[id - 1].first = -1;

            process_id = id / (MAX_INT / world_size);
        }

        // If some parts of the array are not in the current process, send a message to other processes
        if (id != -1)
        {
            Message msg = {process_id, 11, {id, -1}};
            send_queue->push(msg);
            cv.notify_one();

            std::unique_lock<std::mutex> lk(m_get);
            cv_get.wait(lk, []{return get_ready;});
            get_ready = false;
        }
    }

    void DistributedAllocator::async_alloc(unsigned int size)
    {
        int ret_idx = alloc();

        int first_idx = ret_idx;
        if (first_idx == -1)
            return;

        int second_idx;
        unsigned int i = 1;
        int process_id = cur_id / (MAX_INT / world_size);

        // Alloc maximum size in current process
        while ((cur_id < max_id) && (i < size) && (world_rank == process_id))
        {
            // allocate the element
            second_idx = alloc();

            if (second_idx == -1)
                break;

            // link to previous element
            (*collection)[first_idx].first = second_idx;

            first_idx = second_idx;
            process_id = cur_id / (MAX_INT / world_size);
            i++;
        }

        if (process_id >= world_size)
            process_id = 0;

        // If there is no place left, send a message to an other process
        if ((cur_id == max_id) && (i < size))
        {
            // Size left to alloc
            int nsize = size - i;

            Message message = {process_id, 44, {nsize, -1}};
            send_queue->push(message);
            cv.notify_one();
        }
        else
          last = true;

        last_head = ret_idx;
        first_head = first_idx;
    }



    int DistributedAllocator::alloc(unsigned int size)
    {
        int ret_idx = alloc();

        int first_idx = ret_idx;
        if (first_idx == -1)
            return first_idx;


        int second_idx;
        unsigned int i = 1;
        int process_id = cur_id / (MAX_INT / world_size);

        // While there is some place left
        while ((cur_id < max_id) && (i < size) && (world_rank == process_id))
        {
            // allocate element
            second_idx = alloc();

            if (second_idx == -1)
                break;

            // link previous element
            (*collection)[first_idx].first = second_idx;

            first_idx = second_idx;
            process_id = cur_id / (MAX_INT / world_size);
            i++;
        }

        if (process_id >= world_size)
            process_id = 0;

        // if there is no place left
        if ((cur_id == max_id) && (i < size))
        {
            int nsize = size - i;

            demand = true;

            // Ask for some memory to other processes
            Message message = {process_id, 44, {nsize, -1}};
            send_queue->push(message);
            cv.notify_one();

            // Wait until my received thread get the result
            std::unique_lock<std::mutex> lk(m_get);
            cv_get.wait(lk, []{return get_ready;});

            demand = false;
          
            // Getting back the result
            int out = buff_value;
            get_ready = false;
            (*collection)[first_idx].first =  out  ;
        }
        return ret_idx;
    }

    int DistributedAllocator::alloc() {
        int alloc_idx = -1;

        // if we have some free elements
        if (!free_disp->empty())
        {
            // Take a free case from the list
            alloc_idx = free_disp->front();
            (*collection)[alloc_idx] = std::make_pair(-1, 42);
            free_disp->pop();
            return alloc_idx;
        }

        // If we have some place left
        if (cur_id < max_id)
        {
            (*collection)[cur_id] = std::make_pair(-1, 42);
            alloc_idx = cur_id;
            cur_id++;
        }
        else
        {
            // Send a messgage to other processes while we don't have a positive answer
            for (int i = 0; i < world_size; i++)
            {
                // If current process
                if (i == world_rank)
                    continue;

                // Ask for some memory
                Message message = {i, 66, {i, -1}};
                send_queue->push(message);
                cv.notify_one();

                // Wait until my received thread get the result
                std::unique_lock<std::mutex> lk(m_get);
                cv_get.wait(lk, []{return get_ready;});

                // Getting back the result
                int out = buff_value;
                get_ready = false;

                // If we have allocate some memory, break
                if (out != -1)
                {
                    alloc_idx = buff_value;
                    break;
                }
            }
        }

        return alloc_idx;
    }

    int DistributedAllocator::read(int id)
    {
        // Find the process owning the memory
        int process_id = id / (MAX_INT / world_size);

        // If it is us
        if (process_id == world_rank)
            return (*collection)[id].second;

        // Sending message to the owner
        Message msg = {process_id, 99, {id, -1}};
        send_queue->push(msg);

        cv.notify_one();

        // Wait until my received thread get the result
        std::unique_lock<std::mutex> lk(m_get);
        cv_get.wait(lk, []{return get_ready;});

        // Getting back the result
        int out = buff_value;
        get_ready = false;

        return out;
    }

    int DistributedAllocator::next(int id)
    {
        // Find the process owning the memory
        int process_id = id / (MAX_INT / world_size);

        // if it is us
        if (process_id == world_rank)
            return (*collection)[id].first;

        else
        {
            Message msg = {process_id, 101, {id, -1}};
            send_queue->push(msg);

            cv.notify_one();

            // Wait until my received thread get the result
            std::unique_lock<std::mutex> lk(m_get);
            cv_get.wait(lk, []{return get_ready;});

            // getting back the result
            int out = buff_value;
            get_ready = false;
            return out;
        }
    }

    bool DistributedAllocator::write(int id, int value)
    {
        // Find the process owning the memory
        int process_id = id / (MAX_INT / world_size);

        // If is it us
        if (process_id == world_rank)
        {
            (*collection)[id].second = value;
            return true;
        }

        // Send a message to the owner
        Message msg = {process_id, 88, {id, value}};
        send_queue->push(msg);
        cv.notify_one();

        // Wait until my received thread get the result
        std::unique_lock<std::mutex> lk(m_get);
        cv_get.wait(lk, []{return get_ready;});

        // Getting back the result
        bool out = (bool)buff_value;
        get_ready = false;

        return out;
    }

    bool DistributedAllocator::write(int id, int* vals, unsigned int size)
    {
        bool ret = true;
        unsigned int i = 0;
        int next_id = id ;
        while ((i < size) && (next_id != -1))
        {
            ret = ret && write(next_id, *(vals + i));
            next_id = next(next_id);
            i++;
        }

        if ((i != size) && (next_id == -1))
            return false;
        return ret;
    }

    int DistributedAllocator::at(int head, int index)
    {
        int idx = head;
        for (int i = 0; i < index; ++i)
            idx = next(idx);

        return DistributedAllocator::read(idx);
    }

    bool DistributedAllocator::write_at(int head, int index, int value)
    {
        int idx = head;
        for (int i = 0; i < index; ++i)
            idx = next(idx);

        return DistributedAllocator::write(idx, value);
    }
}
