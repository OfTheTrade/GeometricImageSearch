#include <iostream>

#include "../include/parameters.hpp"
#include "../include/dataset.hpp"
#include "../include/vocabulary.hpp"
#include "../include/database.hpp"

#include <filesystem>
#include <opencv2/opencv.hpp>

int main(int argc, char* args[]){
    // Store/Check the input parameters
    Parameters parameters;
    if (!parameters.parseArgv(argc, args)){
        return 1;
    }

    // Store/Check the contents of the splitfile
    Dataset dataset;
    if (!dataset.parseDataset(parameters.split_path, parameters.hp_path)){
        return 1;
    }

    // Train/Load the vocabulary
    Vocabulary vocabulary;

    std::string vocabulary_filename = "vocabulary_cache.xml";
    // If the file is present, load the vocab from there
    if (std::filesystem::exists(vocabulary_filename)){
        std::cerr << "Vocabulary cache file found. Skipped calculating centers manually." << std::endl;
    
        cv::FileStorage storage_file(vocabulary_filename, cv::FileStorage::READ);
        storage_file["centers"] >> vocabulary.centers;
        storage_file.release();
    // If not, calculate it normally
    }else{
        std::cerr << "Vocabulary cache file (" << vocabulary_filename << ") not found. Calculating centers manually." << std::endl;
        
        if (!vocabulary.train(parameters, dataset)){
            return 1;
        }

        // And also store the results for future use
        cv::FileStorage storage_file(vocabulary_filename, cv::FileStorage::WRITE);
        storage_file << "centers" << vocabulary.centers;
        storage_file.release();
    }

    // Construct the database
    Database database;
    if (!database.build(parameters, vocabulary, dataset)){
        return 1;
    }

    // Execute the correct type of search
    if (parameters.searchMethod == SearchMethod::EXACT){
        RunSearch::runExactSearch(database.databaseRecords, database.searchQueries);
    }else if (parameters.searchMethod == SearchMethod::LSH){
        RunSearch::runLSHSearch(database.databaseRecords, database.searchQueries, parameters);
    }else if (parameters.searchMethod == SearchMethod::HYPERCUBE){
        RunSearch::runHypercubeSearch(database.databaseRecords, database.searchQueries, parameters);
    }else if (parameters.searchMethod == SearchMethod::IVFFLAT){
        //RunSearch::runIVFFlatSearch(database.databaseRecords, database.searchQueries, parameters);
    }else if (parameters.searchMethod == SearchMethod::IVFPQ){
        //RunSearch::runIVFPQSearch(database.databaseRecords, database.searchQueries, parameters);
    }

    return 0;
}