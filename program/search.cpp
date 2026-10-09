#include <iostream>

#include "../include/parameters.hpp"

int main(int argc, char* args[]){
    Parameters parameters;
    if (!parameters.parseArgv(argc, args)){
        return 1;
    }
    
    printf("IVFFlat KClusters: (%d)\n", parameters.ivfflat.kclusters);
    if (parameters.set == Set::TEST){
        printf("TEST\n");
    }else if (parameters.set == Set::VALIDATION){
        printf("VALIDATION\n");
    }else{
        printf("PROBLEM\n");
    }
    return 0;
}