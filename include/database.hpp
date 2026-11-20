#ifndef DATABASE_H
#define DATABASE_H

#include "../include/parameters.hpp"
#include "../include/searchmethod.hpp"
#include "../include/vocabulary.hpp"
#include "../include/dataset.hpp"
#include "../include/siftdata.hpp"

// The database given to the search methods
class Database{
    public:
        // Holds the total database (hpatches set + mirflicker D ammount)
        std::vector<DatabaseRecord> databaseRecords;
        std::vector<SearchQuery> searchQueries;

        // Unordered map for quick lookup of database image
        // By image id for records
        std::unordered_map<std::string, SiftData> recordSiftData;
        // By sequence id for queries
        std::unordered_map<std::string, SiftData> querySiftData;

        // Builds the database given to the search methods
        // Return true on success, false on failure
        bool build(const Parameters& parameters, const Vocabulary& vocabulary, const Dataset& dataset);

        // Lookup of the given image_id in the databases unordered map
        // The found sift data struct is output as parameter,
        // Returns true on success, false on failure 
        bool getRecordSiftDataByImageId(const std::string& image_id, SiftData& outputSiftData) const;
        bool getQuerySiftDataBySequenceId(const std::string& sequence_id, SiftData& outputSiftData) const;
};

#endif