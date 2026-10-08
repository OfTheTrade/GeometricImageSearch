#include "../include/parameters.hpp"

bool Parameters::parseArgv(int argc, char* argv[], Parameters& parametes){
    // Used for checking
    bool basicParameterExists[8] = {false};
    bool exactUsed = false;
    bool lshUsed = false;
    bool hypercubeUsed = false;
    bool ivfflatUsed = false;
    bool ivfpqUsed = false;

    for (int i = 1; i < argc; ++i){
        if(argv[i] == "-hp" && i + 1 < argc){

    

        }else{
            printf("Malformed parameter input, call in any order as specified in the pdf!\n");
            return false;
        }
    }

    return true;
}
