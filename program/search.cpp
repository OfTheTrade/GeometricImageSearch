#include <iostream>

#include "../include/parameters.hpp"
#include "../include/dataset.hpp"
#include "../include/vocabulary.hpp"


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

    Dataset dataset;
    if (!dataset.parseDataset(parameters.split_path, parameters.hp_path)){
        return 1;
    }

    for (std::size_t i = 0; i < dataset.train.size(); i++){
        std::cout << dataset.train[i].name << std::endl;
        if (dataset.train[i].set == Set::TEST){
            std:: cout << "TEST" << std::endl;
        }else if(dataset.train[i].set == Set::VALIDATION){
            std:: cout << "VALIDATION" << std::endl;
        }else if(dataset.train[i].set == Set::TRAINING){
            std::cout << "TRAINING" << std::endl;
        }

    }

    Vocabulary vocabulary;
    if (!vocabulary.train(parameters, dataset)){
        return 1;
    }
    
    return 0;
}