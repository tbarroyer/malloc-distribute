This file explain how to use the API:

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

Important: 
- The user has to call the API through the master process which world_rank is 0.
- The default max memory is MAX_INT, it was set for test purposes.

  If you want to be able to allocate the max memory please define de static variable in "src/api/api.cc"
  "# define MAX_INT INT_MAX"

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
