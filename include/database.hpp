#ifndef DATABASE_H
#define DATABASE_H

#include "../include/parameters.hpp"
#include "../include/searchmethod.hpp"
#include "../include/vocabulary.hpp"
#include "../include/dataset.hpp"


class Database{
    public:
        // Holds the total database (hpatches set + mirflicker D ammount)
        std::vector<DatabaseRecord> databaseRecords;
        std::vector<SearchQuery> searchQueries;

        // Builds the database
        // Return true on success, false on failure
        bool build(const Parameters& parameters, const Vocabulary& vocabulary, const Dataset& dataset);
};

#endif