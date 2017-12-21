# include <iostream>
# include <unistd.h>
# include <cstddef>

# include "api/api.hh"

# define SIZE 320

using namespace api;

void swap(int id1, int id2)
{
    int a = DistributedAllocator::read(id1);
    int b = DistributedAllocator::read(id2);

    DistributedAllocator::write(id2, a);
    DistributedAllocator::write(id1, b);
}

int part(int first, int last)
{
    int j = first;

    for (int i = first; i < last; i = DistributedAllocator::next(i))
    {
        if (DistributedAllocator::read(i) <= DistributedAllocator::read(last))
        {
            swap(i, j);
            j = DistributedAllocator::next(j);
        }
    }
    swap(last, j);
    return j;
}

void quick_sort(int first, int last)
{
    if (first < last)
    {
        int p = part(first, last);
        quick_sort(first, p - 1);
        quick_sort(p + 1, last);
    }
}

int main()
{
    DistributedAllocator::init();

    int world_rank = DistributedAllocator::world_rank;

    if (world_rank == 0)
    {
        int array = DistributedAllocator::alloc(SIZE);
        int idx = array;

        for (int i = 0; i < SIZE; ++i)
        {
            DistributedAllocator::write(idx, rand() % SIZE);
            idx = DistributedAllocator::next(idx);
        }

        idx = array;

        for (int i = 0; i < SIZE; ++i)
        {
            std::cout << DistributedAllocator::read(idx) << " ";
            idx = DistributedAllocator::next(idx);
        }

        std::cout << "\n\nBEGIN SORT\n\n";
        quick_sort(array, array + SIZE - 1);
        idx = array;

        for (int i = 0; i < SIZE; ++i)
        {
            std::cout << DistributedAllocator::read(idx) << " ";
            idx = DistributedAllocator::next(idx);
        }
        std::cout << "\n";
    }

    DistributedAllocator::close();
    return 0;
}
