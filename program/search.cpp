#include <iostream>

#include "../include/parameters.hpp"
#include "../include/dataset.hpp"
#include "../include/vocabulary.hpp"
#include "../include/database.hpp"

#include <filesystem>
#include <fstream>
#include <cmath>
#include <opencv2/opencv.hpp>


// If this is turned on, make sure to delete "vocabulary_cache.xml" if any parameters affecting the kmeans,
// like -vocab, -S, -seed or the splitfile are changed between executions.
// I don't want to mess up any automatic checks, so this is off by default
bool USE_VOCABULART_CACHE = true;

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

    // Now that the ammount of database vectors is decided, the default value of 
    // -kclusters can be set, assuming the user did not input a value themselves
    if (parameters.searchMethod == SearchMethod::IVFFLAT && parameters.ivfflat.kclusters < 0){
        parameters.ivfflat.kclusters = static_cast<int>(std::ceil(std::sqrt(database.databaseRecords.size())));
    }else if (parameters.searchMethod == SearchMethod::IVFPQ && parameters.ivfpq.kclusters < 0){
        parameters.ivfpq.kclusters = static_cast<int>(std::ceil(std::sqrt(database.databaseRecords.size())));
    }

    // Execute the correct type of search
    std::vector<SearchResult> results;
    if (parameters.searchMethod == SearchMethod::EXACT){
        results = RunSearch::runExactSearch(database.databaseRecords, database.searchQueries);
    }else if (parameters.searchMethod == SearchMethod::LSH){
        results = RunSearch::runLSHSearch(database.databaseRecords, database.searchQueries, parameters);
    }else if (parameters.searchMethod == SearchMethod::HYPERCUBE){
        results = RunSearch::runHypercubeSearch(database.databaseRecords, database.searchQueries, parameters);
    }else if (parameters.searchMethod == SearchMethod::IVFFLAT){
        results = RunSearch::runIVFFlatSearch(database.databaseRecords, database.searchQueries, parameters);
    }else if (parameters.searchMethod == SearchMethod::IVFPQ){
        results = RunSearch::runIVFPQSearch(database.databaseRecords, database.searchQueries, parameters);
    }

    // Attempt to open/create output file
    // If that fails, default to printing to stdout
    std::ostream* output = &std::cout;
    std::ofstream output_file(parameters.output_path);

    if (output_file.is_open()){
        output = &output_file;
    }else{
        std::cerr << "Output file could not be opened/created. Defaulting output to stdout!" << std::endl;
    }

    // Print the name of the search method used
    if (parameters.searchMethod == SearchMethod::EXACT){
        *output << "Exact Search" << std::endl; 
    }else if (parameters.searchMethod == SearchMethod::LSH){
        *output << "LSH" << std::endl; 
    }else if (parameters.searchMethod == SearchMethod::HYPERCUBE){
        *output << "Hypercube" << std::endl; 
    }else if (parameters.searchMethod == SearchMethod::IVFFLAT){
        *output << "IVFFlat" << std::endl; 
    }else if (parameters.searchMethod == SearchMethod::IVFPQ){
        *output << "IVFPQ" << std::endl; 
    }

    double total_recallAt5 = 0.0;
    double total_recallAt10 = 0.0;
    double total_APAt10 = 0.0;
    double total_timeElapsed = 0.0;

    // Output the individual results per vector item
    for (std::size_t i = 0; i < results.size(); i++){
        SearchResult current_result = results[i];

        *output << "Query: " << current_result.sequence_id << std::endl;
        for (std::size_t j = 0; j < current_result.top10PhotoIds.size(); j++){
            *output << "Rank-" << j + 1 << ": " << current_result.top10PhotoIds[j] << std::endl;
        }
        *output << "Recall@5: " << current_result.RecallAt5 << std::endl
                  << "Recall@10: " << current_result.RecallAt10 << std::endl
                  << "AP@10: " << current_result.APAt10 << std::endl;

        total_recallAt5 += current_result.RecallAt5;
        total_recallAt10 += current_result.RecallAt10;
        total_APAt10 += current_result.APAt10;
        total_timeElapsed += current_result.timeElapsed;
    }

    // Output the mean results
    std::size_t total_results = results.size();
    if (total_results > 0){
        *output << "Mean Recall@5: " << total_recallAt5 / total_results << std::endl
                << "Mean Recall@10: " << total_recallAt10 / total_results << std::endl
                << "mAP@10: " << total_APAt10 / total_results << std::endl
                << "Average query time: " << total_timeElapsed / total_results << std::endl;
    }else{
        *output << "No search results found" << std::endl;
    }

    if (output_file.is_open()){
        output_file.close();
    }

    return 0;
}