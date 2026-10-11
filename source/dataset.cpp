#include "../include/dataset.hpp"

#include <fstream>
#include <sstream>

// Parses the split file and stores it's information in the struct
bool Dataset::parseDataset(std::string filename, std::string hp_path){
    std::ifstream splitfile(filename);
    if (!splitfile.is_open()){
        fprintf(stderr, "Splitfile not found/can't be opened!\n");
        return false;
    }
    std::string line;

    // Read line by line from the split file
    while (getline(splitfile,line)){

        if(line.empty()) continue;

        // Take the first two items from each line
        std::istringstream iline(line);
        std::string name, set;

        if (iline >> name >> set){  
            // Add this lines info to the datasetset class
            HPatchesSequence sequence;
            sequence.name = name;
            sequence.path = hp_path + "/" + name;

            if (set == "train"){
                
                sequence.set = Set::TRAINING;
                this->train.push_back(sequence);
            }else if (set == "validation"){

                HPatchesSequence sequence;
                sequence.name = name;
                sequence.set = Set::VALIDATION;
                this->validation.push_back(sequence);
            }else if (set == "test"){

                HPatchesSequence sequence;
                sequence.name = name;
                sequence.set = Set::TEST;
                this->test.push_back(sequence);
            }
        }
    }
    
    splitfile.close();
    return true;
}