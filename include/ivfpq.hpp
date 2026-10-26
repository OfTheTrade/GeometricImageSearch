#ifndef IVFPQ_HPP
#define IVFPQ_HPP

#include <vector>
#include <string>
#include <cstdint>
#include "parameters.hpp"
#include "database.hpp"
#include "searchmethod.hpp"

// Quantized record stored inside each inverted list
struct QuantizedRecord {
    std::string image_id;
    std::vector<uint8_t> codes; // M codes (1 byte per sub-vector for nbits <= 8)
};

// Inverted list corresponding to a single coarse cluster
struct InvertedListPQ {
    std::vector<QuantizedRecord> entries;
};

class IVFPQIndex {
public:
    int K_dim;       // Vector dimensionality (vocabulary size)
    int k_coarse;    // Number of coarse clusters (kclusters)
    int nprobe;      // Number of coarse centroids to inspect during search
    int M;           // Number of sub-vectors for product quantization
    int d_sub;       // Dimensionality of each sub-vector: K_dim / M
    int nbits;       // Bits per sub-vector code (default: 8 -> 256 centroids)
    int k_sub;       // 2^nbits (e.g., 256)

    // Coarse centroids: size [k_coarse][K_dim]
    std::vector<std::vector<float>> coarse_centroids;

    // Inverted lists: one per coarse cluster
    std::vector<InvertedListPQ> inverted_lists;

    // PQ Codebooks: for each subspace m in [0, M-1], k_sub centroids of dimension d_sub
    // Dimensions: [M][k_sub][d_sub]
    std::vector<std::vector<std::vector<float>>> pq_codebooks;

    IVFPQIndex(int kclusters, int nprobe_val, int M_val, int nbits_val, int dim);

    void trainAndBuild(const std::vector<DatabaseRecord>& records, int seed);

    std::vector<std::pair<double, std::string>> query(const std::vector<float>& query_vec) const;
};

#endif // IVFPQ_HPP