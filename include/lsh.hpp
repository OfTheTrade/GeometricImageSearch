#pragma once    
#include <vector>
#include <string>
#include <unordered_map>
#include <random>
#include <cstdint>
#include "dataset.hpp"
#include "searchmethod.hpp"


struct L2HashFunction {
    std::vector<float> v; // gaussian random vector
    float t;              // uniform in [0,w)
    float w;              // window size

    int32_t compute(const std::vector<float>& x) const;
};


struct LSHCompositeFunction {
    std::vector<L2HashFunction> h_funcs;
    std::vector<uint32_t> r; // random weights for linear combination
    static const uint32_t M = 4294967291u; // 2^32 - 5

    uint32_t computeID(const std::vector<float>& x) const;
};


class LSHHashTable {
public:
    LSHCompositeFunction g; // composite hash function
    std::unordered_map<uint32_t, std::vector<size_t>> buckets;

    void insert(size_t index, const std::vector<float>& vector);
    const std::vector<size_t>* getCandidates(const std::vector<float>& vector, uint32_t& out_id) const;
};


class LSHIndex {
public:
    int L;
    int k;
    float w;
    int dim;
    std::vector<LSHHashTable> tables;
    const std::vector<DatabaseRecord>& db;

    LSHIndex(int L, int k, float w, int dim, unsigned int seed, const std::vector<DatabaseRecord>& db);
    std::vector<size_t> queryCandidates(const std::vector<float>& query_vector) const;
};