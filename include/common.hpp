#ifndef COMMON_H
#define COMMON_H

// Enum for the search methods
enum class SearchMethod{
    EXACT,
    LSH,
    HYPERCUBE,
    IVFFLAT,
    IVFPQ
};

// Enum for the sets
enum class Set{
    VALIDATION,
    TEST,
    TRAINING
};

#endif