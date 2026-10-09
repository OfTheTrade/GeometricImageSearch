#ifndef PARAMETERS_H
#define PARAMETERS_H

#include <string>

// Enum for the search methods
enum class SearchMethod{
    EXACT,
    LSH,
    HYPERCUBE,
    IVFFLAT,
    IVFPQ
};

enum class Set{
    VALIDATION,
    TEST
};

// Contains all parameters as they are defined in the original pdf
class Parameters{
    public:

        // ===================================
        //          General Variables
        // ===================================

        // Paths to required files
        std::string hp_path;
        std::string mir_path;
        std::string split_path;
        // K, ammount of centers
        int vocab;
        // Max SWIFT descriptor ammount
        int S;
        // MIRFlicker pic ammount
        int D;
        // "Validation" or "Test"
        Set set;
        // Path to output
        std::string output_path;
        // Seed for pseudorandomness
        int seed;

        // The search method picked for this execution
        SearchMethod searchMethod;

        // ===================================
        //      Search Specific Variables
        // ===================================

        struct LSHParameters{
            public:
                // Ammount of h_i functions making up each fragmentation function g
                int k = 4;
                // Ammount of fragmentation matrixes
                int L = 5;
                // Length of the cell on the projection line
                double w = 0.05;
        };

        struct HypercubeParameters{
            public:
                // Dimension d' of the hypercube
                int kproj = 10;
                // Length of the cell on the projection line
                double w = 0.05;
                // Maximun number of potential points that are checked
                int M = 500;
                // Maximun number of vertices of the hypercube that are checked
                int probes = 10;
        };

        // Note: Both IVF methods, in order to calculate some of their default values,
        // require n to be calculated, which happens later in the program. This does
        // not happen if they are actually given a non default value from the command line 

        struct IVFFlat{
            public:
                // Number of clusters in the index
                // Actual default value (ceil(sqrt(n))) need to be calculated later
                int kclusters = -1;
                // Number of nearest clusters checked during the search
                int nprobe = 5;
        };

        struct IVFPQ{
            public:
                // Number of clusters in the index
                // Actual default value (ceil(sqrt(n))) need to be calculated later
                int kclusters = -1;
                // Number of nearest clusters checked during the search
                int nprobe = 5;
                // Number of subimages of product quantization
                // K must be exactly divisible by M
                int M = 16;
                // Each subspace is clustered into 2^(nbits) clusters
                // Default Clusters: 256
                int nbits = 8;
        };

        LSHParameters lsh;
        HypercubeParameters hypercube;
        IVFFlat ivfflat;
        IVFPQ ivfpq;

        // Parses args and fills out the parameter struct
        // True on success, False on failure
        bool parseArgv(int argc, char* argv[]);
};

#endif


