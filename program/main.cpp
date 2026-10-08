#include <iostream>

#include "../include/parameters.hpp"

int main(int argc, char* args[]){
    std::cout << "Arr" << std::endl;
    Parameters parameters;
    if (!Parameters::parseArgv(argc, args, parameters)){
        return 0;
    }
    return 0;
}