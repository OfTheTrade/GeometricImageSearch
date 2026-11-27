#ifndef FEATUREMATCHING_H
#define FEATUREMATCHING_H

#include "../include/verification.hpp"
#include "../include/searchmethod.hpp"
#include "../include/database.hpp"
#include "../include/parameters.hpp"

// Contains the results of the feature matching procedure
struct FeatureMatcher{
    std::vector<QueryCorrespondance>& queryCorrespondances;

    // Execute the matching and store the results in this struct
    // True on success, false on failure
    bool runMatching(const std::vector<SearchResult>& searchResults, const Database& database, const Parameters& parameters);
};

#endif