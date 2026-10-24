#ifndef DATABASE_H
#define DATABASE_H

#include "../include/parameters.hpp"
#include "../include/searchmethod.hpp"
#include "../include/vocabulary.hpp"
#include "../include/dataset.hpp"

// The database given to the search methods
class Database{
    public:
        // Holds the total database (hpatches set + mirflicker D ammount)
        std::vector<DatabaseRecord> databaseRecords;
        std::vector<SearchQuery> searchQueries;

        // Builds the database given to the search methods
        // Return true on success, false on failure
        bool build(const Parameters& parameters, const Vocabulary& vocabulary, const Dataset& dataset);
};

#endif