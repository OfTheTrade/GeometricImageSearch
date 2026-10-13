#ifndef SEARCHMETHOD_H
#define SEARCHMETHOD_H

#include "../include/parameters.hpp"

#include <vector>

// Contains the images that make up the working database
struct DatabaseRecord{
    // Format "HPatches folder name"/"image number (0-5)" 
    std::string image_id;
    // Frequency vector
    std::vector<float> vector;
};

// Contains a single query
struct SearchQuery{
    // HPatches folder
    std::string sequence_id;
    // Frequency vector
    std::vector<float> vector;
};

// Contains the results of a single query
struct SearchResult{
    // HPatches folder
    std::string sequence_id;
    // Top 10 ranked images
    std::vector<std::string> top10PhotoIds;
    
    // Comparison Metrics
    double RecallAt5 = 0.0;
    double RecallAt10 = 0.0;
    double APAt10 = 0.0;
    double timeElapsed = 0.0;
};


// Will contain all types of searches as static methods
class RunSearch{
    public:
        static std::vector<SearchResult> runExactSearch(
            const std::vector<DatabaseRecord> database,
            const std::vector<SearchQuery> queries
        );

        static std::vector<SearchResult> runLSHSearch(
            const std::vector<DatabaseRecord> database,
            const std::vector<SearchQuery> queries,
            const Parameters parameters
        );

        static std::vector<SearchResult> runHypercubeSearch(
            const std::vector<DatabaseRecord> database,
            const std::vector<SearchQuery> queries,
            const Parameters parameters
        );

        static std::vector<SearchResult> runIVFFlatSearch(
            const std::vector<DatabaseRecord> database,
            const std::vector<SearchQuery> queries,
            const Parameters parameters
        );
        
        static std::vector<SearchResult> runIVFPQSearch(
            const std::vector<DatabaseRecord> database,
            const std::vector<SearchQuery> queries,
            const Parameters parameters
        );
};

#endif