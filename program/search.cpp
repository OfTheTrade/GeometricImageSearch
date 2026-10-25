#include <iostream>

#include "../include/parameters.hpp"
#include "../include/dataset.hpp"
#include "../include/vocabulary.hpp"
#include "../include/database.hpp"

#include <filesystem>
#include <opencv2/opencv.hpp>


// If this is turned on, make sure to delete "vocabulary_cache.xml" if any parameters affecting the kmeans,
// like -K, -S, -seed or the splitfile are changed between executions.
// I don't want to mess up any automatic checks, so this is off by default
bool USE_VOCABULART_CACHE = false;

int main(int argc, char* args[]){
    // Store/Check the input parameters
    Parameters parameters;
    if (!parameters.parseArgv(argc, args)){
        return 1;
    }

    // Store/Check the contents of the splitfile
    Dataset dataset;
    if (!dataset.parseDataset(parameters)){
        return 1;
    }

    // Train/Load the vocabulary
    Vocabulary vocabulary;

    std::string vocabulary_filename = "vocabulary_cache.xml";
    // If the file is present, load the vocab from there
    if (std::filesystem::exists(vocabulary_filename) && USE_VOCABULART_CACHE){
        std::cerr << "Vocabulary cache file (" << vocabulary_filename << ") found. Skipped calculating centers manually."
        << std:: endl << "(Make sure to delete the cache file if you want to use different parameters that affect the vocabulary training, like -S, -K, -seed or the splitfile)" << std::endl;
    
        cv::FileStorage storage_file(vocabulary_filename, cv::FileStorage::READ);
        storage_file["centers"] >> vocabulary.centers;
        storage_file.release();
    // If not, calculate it normally
    }else{
        if (USE_VOCABULART_CACHE == true) std::cerr << "Vocabulary cache file (" << vocabulary_filename << ") not found. ";
        std::cerr << "Calculating centers manually." << std::endl;
    
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
        RunSearch::runIVFFlatSearch(database.databaseRecords, database.searchQueries, parameters);
    }else if (parameters.searchMethod == SearchMethod::IVFPQ){
        //RunSearch::runIVFPQSearch(database.databaseRecords, database.searchQueries, parameters);
    }

    return 0;
}