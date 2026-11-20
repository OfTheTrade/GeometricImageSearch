#include "../include/database.hpp"

#include "../include/vectorizer.hpp"

#include <utility>

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
        SiftData query_sift_data;
        
        std::string qr_img_path = hpatches_sequences[i].path + "/1.ppm";

        if (Vectorizer::vectorizeImage(qr_img_path, query.vector, query_sift_data, vocabulary)){
            this->searchQueries.push_back(query);
            this->querySiftData[query.sequence_id] = std::move(query_sift_data);
        }else{
            std::cerr << "Warning : Unable to vectorize query image" << std::endl;
        }

        // Second to sixth image are put in the database
        for (int j = 2; j <= 6; j++){
            DatabaseRecord hp_record;
            hp_record.image_id = hpatches_sequences[i].name + "/" + std::to_string(j);
            SiftData hp_sift_data;

            std::string hp_img_path = hpatches_sequences[i].path + "/" + std::to_string(j) + ".ppm";

            if (Vectorizer::vectorizeImage(hp_img_path, hp_record.vector, hp_sift_data, vocabulary)){
                this->recordSiftData[hp_record.image_id] = std::move(hp_sift_data);
                hp_records.push_back(hp_record);
            }else{
                std::cerr << "Warning : Unable to vectorize database image" << std::endl; 
            }

        }
    }

    // Holds the mir type records of the database
    std::vector<DatabaseRecord> mir_records;
    mir_records.reserve(parameters.D);
    
    int mir_index = 1;

    // Process D mirflicker distractors, putting them into the database
    while (mir_records.size() < static_cast<size_t>(parameters.D)){  
        DatabaseRecord mir_record;
        mir_record.image_id = "im" + std::to_string(mir_index) + ".jpg";
        SiftData mir_sift_data;
        
        std::string mir_img_path = parameters.mir_path + "/im" + std::to_string(mir_index) + ".jpg";

        if (Vectorizer::vectorizeImage(mir_img_path, mir_record.vector, mir_sift_data, vocabulary)){
            this->recordSiftData[mir_record.image_id] = std::move(mir_sift_data);
            mir_records.push_back(mir_record);
        }else{
            std::cerr << "Warning : Unable to vectorize mirflicker image" << std::endl;
        }

        mir_index++;

        if (mir_index > 25000){
            std::cerr << "Run out of mirflicker images before gathering the asked ammount" << std::endl;
            break;
        }
    }
         
    
    // Combine the mirflicker and hpatches databases into the final database 
    this->databaseRecords.reserve(hp_records.size() + mir_records.size());
    this->databaseRecords.insert(databaseRecords.end(), hp_records.begin(), hp_records.end());
    this->databaseRecords.insert(databaseRecords.end(), mir_records.begin(), mir_records.end());

    std::cerr << "Database built!" << std::endl 
              << "Search Queries: (" << this->searchQueries.size() << ")" << std::endl
              << "HPatches Records: (" << hp_records.size() << ")" << std::endl  
              << "MirFlicker Records: (" << mir_records.size() << ")" << std::endl
              << "Total Records: (" << this->databaseRecords.size() << ")" << std::endl;
            
    return true;
}

bool Database::getRecordSiftDataByImageId(const std::string& image_id, SiftData& outputSiftData) const{
    std::unordered_map<std::string, SiftData>::const_iterator it = this->recordSiftData.find(image_id);
    if (it == this->recordSiftData.end()){
        return false;
    }
    outputSiftData = it->second;
    return true;
}

bool Database::getQuerySiftDataBySequenceId(const std::string& sequence_id, SiftData& outputSiftData) const{
    std::unordered_map<std::string, SiftData>::const_iterator it = this->querySiftData.find(sequence_id);
    if (it == this->querySiftData.end()){
        return false;
    }
    outputSiftData = it->second;
    return true;
}