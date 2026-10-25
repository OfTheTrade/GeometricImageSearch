#ifndef DATASET_H
#define DATASET_H

#include "../include/common.hpp"
#include "../include/parameters.hpp"

#include <string>
#include <vector>

// Represents a folder in the hpatches directory
class HPatchesSequence{
    public:
        // Just the name
        std::string name;
        // Path + Name
        std::string path;
        Set set;

};

// Stores the info of the splitfile
class Dataset{
    public:
        std::vector<HPatchesSequence> train;
        std::vector<HPatchesSequence> validation;
        std::vector<HPatchesSequence> test;

        // Parses the split file and stores it's information in the struct
        bool parseDataset(const Parameters& parameters);
};

#endif