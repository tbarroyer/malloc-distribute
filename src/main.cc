# include <iostream>

# include "api/api.hh"

int main() {
    api::init();
    api::print();
    api::close();
    return 0;
}
