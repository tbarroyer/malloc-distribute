# pragma once

# include <mpi.h>
# include <cstddef>
# include <map>
# include <thread>
# include <mutex>
# include <queue>
# include <utility>
# include <condition_variable>
# include <vector>

namespace api {

    struct Message {
        int process_id;
        int tag;
        int data[2];
    };

    class DistributedAllocator {
    public:
        // init OPENMPI
        static void init();

        // close OPENMPI
        static void close();

        // allocate space for an array
        // return id of the integer
        static int alloc(unsigned int size);
       
        // asynchronous allocation for an array
        static void async_alloc(unsigned int size);

        // allocation
        // return the id of the integer
        static int alloc();

        // Getting the next id for an array
        static int next(int id);

        // read in memory
        static int read(int id);

        // getting value in an array at a specified index
        static int at(int head, int index);

        // free memory
        static void free(int id);


        // write in memory
        static bool write(int id, int value);

        // write a Table in memory
        static bool write(int id, int* vals,unsigned int size);
    
        // write at a position in an array
        static bool write_at(int head, int index, int value);

    private:
        // Received loop
        static void loop_re();
        // Send loop
        static void loop_se();

    public:
        // Number of process
        static int world_size;
        // Id of process
        static int world_rank;
        // Queue of available memory
        static std::queue<int>* free_disp;
    private:
        // Buffer to talk with threads
        static int  buff_value;
        // Waike up a thread
        static bool get_ready;

        // Max allocation id
        static int max_id;
        // Current allocation id
        static int cur_id;

        // Collection id : <next element, value>
        static std::map<int, std::pair<int, int>>* collection;

        // Sending message queue
        static std::queue<Message>* send_queue;

        // Receiving thread
        static std::thread re;
        // Sending thread
        static std::thread se;

        // Two mutex to make thread blocks
        static std::mutex m;
        static std::mutex m_get;

        // Two conditional variable to waike threads up
        static std::condition_variable cv;
        static std::condition_variable cv_get;

        // Communication with threads
        static int  last_head;
        static int  first_head;
        static bool last;
        static bool demand;
    };
}
