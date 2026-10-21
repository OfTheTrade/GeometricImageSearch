#pragma once
#include <vector>
#include <unordered_map>
#include <random>
#include <cstdint>
#include "searchmethod.hpp"

// single projection function h(x) mapped to a bit {0,1}
class HypercubeHashFunction {
public:
    std::vector<float> v; // random vector
    float t; // random offset
    float w; // width of the projection
    mutable std::unordered_map<int32_t, uint8_t> f_map; // cache for computed hash values
    mutable std::mt19937 rng; // random number generator
    mutable std::uniform_int_distribution<int> bit_dist; // distribution for generating bits

    HypercubeHashFunction() : t(0.0f), w(1.0f), bit_dist(0, 1) {}
    void init(int dim, float window, unsigned int seed);
    uint8_t computeBit(const std::vector<float>& x) const;
};

// index structure for the hypercube
class HypercubeIndex {
public:
    int kproj; // d' dimentions
    int M; // max items to check
    int probes; // max vertices to visit
    float w; // width of the projection
    const std::vector<DatabaseRecord>& db;
    std::vector<HypercubeHashFunction> h_funcs;
    std::unordered_map<uint32_t, std::vector<size_t>> cube_vertices;

    HypercubeIndex(int kproj, int M, int probes, float w , int dim, unsigned int seed, const std::vector<DatabaseRecord>& database);
    uint32_t computeVertex(const std::vector<float>& x) const;
    std::vector<size_t> queryCandidates(const std::vector<float>& q_vec) const;
};