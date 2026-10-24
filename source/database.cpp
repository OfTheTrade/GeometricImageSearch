#include "../include/database.hpp"

#include "../include/vectorizer.hpp"

// Builds the database given to the search methods
// Return true on success, false on failure
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

    // Hold the hp type records of the database
    std::vector<DatabaseRecord> hp_records;

    // Process the hpatches sequences into the database
    for (size_t i = 0; i < hpatches_sequences.size(); i++){
        // First picture are put in the query vector
        SearchQuery query;
        query.sequence_id = hpatches_sequences[i].name;
        
        std::string qr_img_path = hpatches_sequences[i].path + "/1.ppm";

        if (Vectorizer::vectorizeImage(qr_img_path, vocabulary.centers, query.vector)){
            this->searchQueries.push_back(query);
        }else{
            std::cout << "Warning : Unable to vectorize query image" << std::endl;
        }

        // Second to sixth image are put in the database
        for (int j = 2; j <= 6; j++){
            DatabaseRecord hp_record;
            hp_record.image_id = hpatches_sequences[i].name + "/" + std::to_string(j);

            std::string hp_img_path = hpatches_sequences[i].path + "/" + std::to_string(j) + ".ppm";

            if (Vectorizer::vectorizeImage(hp_img_path, vocabulary.centers, hp_record.vector)){
                hp_records.push_back(hp_record);
            }else{
                std::cout << "Warning : Unable to vectorize database image" << std::endl; 
            }

        }
    }

    // Holds the mir type records of the database
    std::vector<DatabaseRecord> mir_records;
    mir_records.reserve(parameters.D);
    
    int mir_index = 1;

    // Process D mirflicker distractors, putting them into the database
    if (false){
        while (mir_records.size() < static_cast<size_t>(parameters.D)){  
            DatabaseRecord mir_record;
            mir_record.image_id = "im" + std::to_string(mir_index);
            
            std::string mir_img_path = parameters.mir_path + "/im" + std::to_string(mir_index) + ".jpg";

            if (Vectorizer::vectorizeImage(mir_img_path, vocabulary.centers, mir_record.vector)){
                mir_records.push_back(mir_record);
            }else{
                std::cout << "Warning : Unable to vectorize mirflicker image" << std::endl;
            }

            mir_index++;

            if (mir_index > 25000){
                std::cout << "Run out of mirflicker images before gathering the asked ammount" << std::endl;
                break;
            }
        }
        
    }
    // Combine the mirflicker and hpatches databases into the final database 
    this->databaseRecords.reserve(hp_records.size() + mir_records.size());
    this->databaseRecords.insert(databaseRecords.end(), hp_records.begin(), hp_records.end());
    this->databaseRecords.insert(databaseRecords.end(), mir_records.begin(), mir_records.end());

    std::cout << "Database built!" << std::endl 
              << "Search Queries: (" << this->searchQueries.size() << ")" << std::endl
              << "HPatches Records: (" << hp_records.size() << ")" << std::endl  
              << "MirFlicker Records: (" << mir_records.size() << ")" << std::endl
              << "Total Records: (" << this->databaseRecords.size() << ")" << std::endl;
            
    return true;
}