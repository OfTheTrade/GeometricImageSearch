#ifndef DATASET_H
#define DATASET_H

#include "../include/common.hpp"

#include <string>
#include <vector>

// Represents a folder in the hpatches directory
class HPatchesSequence{
    public:
        std::string name;
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
        bool parseDataset(std::string filename, std::string hp_path);
};

#endif