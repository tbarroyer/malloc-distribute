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

        // allocate space for an integer
        // return id of the interger
        static int alloc(unsigned int size);
        
        static void async_alloc(unsigned int size);

        static int alloc();

        static int next(int id);

        // read in memory
        static int read(int id);

        static int at(int head, int index);

        // free memory
        static void free(int id);


        // write in memory
        static bool write(int id, int value);

        // write a Table in memory
        static bool write(int id, int* vals,unsigned int size);
    
        static bool write_at(int head, int index, int value);

    private:
        static void loop_re();
        static void loop_se();

    public:
        // Number of process
        static int world_size;
        // Id of process
        static int world_rank;
        static std::queue<int>* free_disp;
    private:
        static int  buff_value;
        static bool get_ready;

        static int max_id;
        static int cur_id;
        static std::map<int, std::pair<int, int>>* collection;

        static std::queue<Message>* send_queue;

        //static std::queue<int>* free_disp;

        static std::thread re;
        static std::thread se;

        static std::mutex m;
        static std::mutex m_get;

        static std::condition_variable cv;
        static std::condition_variable cv_get;

        static int last_head;
        static int first_head;
        static bool last;
        static bool demand;
    };
}
