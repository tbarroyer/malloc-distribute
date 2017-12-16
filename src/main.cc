# include <iostream>

# include "api/api.hh"

using namespace api;

int main() {
    DistributedAllocator::init();
    DistributedAllocator::close();
    return 0;
}
