#pragma once

#include <vector>
#include <string>
#include <cstdint>
#include "searchmethod.hpp"

class IVFFlatIndex {
public:
    int k_clusters;
    int nprobe;
    int dim;
    const std::vector<DatabaseRecord>& db;

    //centroids of clusters: shape [k_clusters][dim]
    std::vector<std::vector<float>> centroids;

    //inverted lists: cluster_id -> list of indices in database
    std::vector<std::vector<size_t>> inverted_lists;

    IVFFlatIndex(int k_clusters, int nprobe, int dim, unsigned int seed, const std::vector<DatabaseRecord>& database);
    std::vector<size_t> queryCandidates(const std::vector<float>& q_vec) const;

private:
    void trainKMeans(unsigned int seed, int max_iterations = 20);
};