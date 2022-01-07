#include "resman.hpp"

using namespace chen;

int main()
{
    Resman resman;
    bool result = resman.packFolder(".");
    if(result) resman.loadResourceFile();
    // auto file = resman.getFile("./booba.txt");
    // if(file.has_value())
    // {
    //     std::cout << file.value().path << std::endl;
    //     std::cout << file.value().size << std::endl;
    //     std::cout << file.value().data << std::endl;
    //     return 0;
    // }
    else
    return 0;
}