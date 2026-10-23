#include "../include/database.hpp"

bool Database::build(const Parameters& parameters, const Vocabulary& vocabulary, const Dataset& dataset){
    
    // According to the input parameters, use the correct type of hpatches sequences
    std::vector<HPatchesSequence> hpatches_sequences;
    if (parameters.set == Set::TEST){
        hpatches_sequences = dataset.test;
    }else if(parameters.set == Set::VALIDATION){
        hpatches_sequences = dataset.validation;
    }else{
        std::cerr << "HPatches -set should be (validation) or (test). Current one unrecognised." << std::endl;
        return false;
    }
    return true;
}