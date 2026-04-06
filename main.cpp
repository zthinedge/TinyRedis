#include "include/core/sds.hpp"

#include <iostream>

int main()
{
    SDS s("hello");

    s.append(" world",6);

    std::cout<<s.c_str()<<std::endl;
    std::cout<<"len="<<s.len()<<std::endl;
    std::cout<<"cap="<<s.capacity()<<std::endl;

    s.append(" redis sds",10);

    std::cout<<s.c_str()<<std::endl;
}