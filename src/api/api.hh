# pragma once

# include <mpi.h>
# include <cstddef>
# include <map>
# include <thread>
# include <mutex>
# include <queue>
# include <utility>
# include <condition_variable>

namespace api {
    class DistributedAllocator {
    public:
        // init OPENMPI
        static void init();

        // close OPENMPI
        static void close();

        // allocate space for an integer
        // return id of the interger
        static unsigned int alloc();

        // read in memory
        static int read(unsigned int id);

    private:
        static void loop_re();
        static void loop_se();

    public:
        // Number of process
        static int world_size;
        // Id of process
        static int world_rank;

    private:
        static unsigned int max_id;
        static std::map<unsigned int, int>* collection;
        static std::queue<std::pair<int, int>>* send_value;
//        static std::queue<int>* send_key;
        static std::thread re;
        static std::thread se;

        static std::mutex m;
        static std::condition_variable cv;
    };
}
