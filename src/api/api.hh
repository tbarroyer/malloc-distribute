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
    class DistributedAllocator {
    public:
        // init OPENMPI
        static void init();

        // close OPENMPI
        static void close();

        // allocate space for an integer
        // return id of the interger
        static int alloc(unsigned int size);
        static int alloc();

        // read in memory
        static int read(int id);


        // free memory
        static void free(int id);


        // write in memory
        static bool write(int id, int value);

    private:
        static void loop_re();
        static void loop_se();

    public:
        // Number of process
        static int world_size;
        // Id of process
        static int world_rank;

    private:
        static int  buff_value;
        static bool get_ready;

        static int max_id;
        static int cur_id;
        static std::map<int, std::pair<int, int>>* collection;

        static std::queue<std::pair<int, int>>* send_value;
        static std::queue<std::pair<int, int>>* send_free;
        static std::queue<std::pair<int, int>>* ret_free;
        static std::queue<std::pair<int, int>>* send_key;
        static std::queue<std::pair<int, int>>* send_alloc_req;
        static std::queue<std::pair<int, int>>* send_alloc_resp;
        static std::queue<std::pair<int, std::pair<int, int>>>* send_key_write;

        static std::vector<int>* free_disp;

        static std::thread re;
        static std::thread se;

        static std::mutex m;
        static std::mutex m_get;

        static std::condition_variable cv;
        static std::condition_variable cv_get;
    };
}
