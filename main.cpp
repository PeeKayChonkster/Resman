#include "resman.hpp"

int main()
{
    chen::Resman resman;
    //bool result = chen::Resman::packFolder(".");
    auto file = resman.getFile("./booba.txt");
    if(file.has_value())
    {
        std::cout << file.value().path << std::endl;
        std::cout << file.value().size << std::endl;
        std::cout << file.value().data << std::endl;
        return 0;
    }
    else
    return 1;
}