This file explain how to use the API:
    Please use make check for a simple run of the program

void init();
	This function initializes a DistributedAllocator dans must be call by all the processes.

void close();
	This function closes the threads of each process and terminate it.

int alloc(unsigned int size);
	This function allocates size int on the shared memory and return the head of the collection.

int alloc();
	This function allocates a single int on the shared memory and return his id.

int next(int id);
	This function neturns the id of the next element into a collection.
	If -1 is return it's the last element of the collection.

int read(int id);
	This function reads the value of the id given as input.

static void free(int id);
	This function frees the value corresponding to the id given as input.
	If the passed id is the head of a collection, it frees all of it.

static bool write(int id, int value);
	This function writes the value in the corresponding variable to id.

---------------------------------------------------------------------------------------------------

CAUTION:
- Please don't use more process than core you have on your machine, or the programm will be very slow.
  Because MPI can emulate more processes than cores, on low peformance processor it result in a very poor performance.
  We experienced very strange behaviors on different configuration. If you have two cores on you machine please use 2 process.
  And four process if your have an i7 for example.
- Please inform us if the code doesn't work on your machine, we noticed that for unknown reason it can't work on some configuration
- The user has to call the API through the master process which world_rank is 0.
- The default max memory is MAX_INT, it was set for test purposes.

  If you want to be able to allocate the max memory please define de static variable in "src/api/api.cc"
  "# define MAX_INT INT_MAX"

--------------------------------------------------------------------------------------------------
Mandatory calls are:

int main()
{
    DistributedAllocator::init();

    int world_rank = DistributedAllocator::world_rank;

    if (world_rank == 0)
    {
	//CODE HERE
    }

    DistributedAllocator::close();
    return 0;
}

Code example:

int main()
{
    // Initialization must be call at the beginning outside an "if (world_rank == 0)"
    // to init all the DistributedAllocators on every process
    DistributedAllocator::init();

    int world_rank = DistributedAllocator::world_rank;

    if (world_rank == 0)
    {
	// Get the head of the collection
        int head = DistributedAllocator::alloc(SIZE);

	// Define a variable to loop through the collection without losing the head
        int idx = head;

        for (int i = 0; i < SIZE; ++i)
        {
	    // Write "value" to the "idx" index of the collection
            DistributedAllocator::write(idx, value);

	    // Get the id of the next element
            idx = DistributedAllocator::next(idx);
        }

	// Go to the head again
        idx = head;

        for (int i = 0; i < SIZE; ++i)
        {
	    // Read the values of the variable "idx"
            std::cout << DistributedAllocator::read(idx) << " ";

	    // Get the id of the next element
            idx = DistributedAllocator::next(idx);
        }
	// Free all the collection the allocate up there
	DistributedAllocator::free(head);
	
    }

    // Close all the processes
    DistributedAllocator::close();
    return 0;
}
