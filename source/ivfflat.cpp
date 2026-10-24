#include "../include/ivfflat.hpp"
#include <cmath>
#include <chrono>
#include <queue>
#include <algorithm>
#include <random>
#include <limits>

static inline float euclideanDistanceSquared(const std::vector<float>& a, const std::vector<float>& b) {
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

void IVFFlatIndex::trainKMeans(unsigned int seed, int max_iterations) {
    if (db.empty() || k_clusters <= 0) return;

    std::mt19937 rng(seed);
    std::vector<size_t> indices(db.size());
    for (size_t i = 0; i < db.size(); ++i) indices[i] = i;
    std::shuffle(indices.begin(), indices.end(), rng);

    // Initial centroid selection (k random distinct points)
    centroids.resize(k_clusters);
    for (int i = 0; i < k_clusters; ++i) {
        centroids[i] = db[indices[i % db.size()]].vector;
    }

    std::vector<int> assignments(db.size(), -1);

    // Lloyd's algorithm iterations
    for (int iter = 0; iter < max_iterations; ++iter) {
        bool changed = false;

        // Assignment step
        for (size_t i = 0; i < db.size(); ++i) {
            float min_dist = std::numeric_limits<float>::max();
            int best_cluster = 0;

            for (int c = 0; c < k_clusters; ++c) {
                float dist = euclideanDistanceSquared(db[i].vector, centroids[c]);
                if (dist < min_dist) {
                    min_dist = dist;
                    best_cluster = c;
                }
            }

            if (assignments[i] != best_cluster) {
                assignments[i] = best_cluster;
                changed = true;
            }
        }

        if (!changed && iter > 0) break;

        //update step
        std::vector<std::vector<float>> new_centroids(k_clusters, std::vector<float>(dim, 0.0f));
        std::vector<int> counts(k_clusters, 0);

        for (size_t i = 0; i < db.size(); ++i) {
            int c = assignments[i];
            counts[c]++;
            for (int d = 0; d < dim; ++d) {
                new_centroids[c][d] += db[i].vector[d];
            }
        }

        for (int c = 0; c < k_clusters; ++c) {
            if (counts[c] > 0) {
                for (int d = 0; d < dim; ++d) {
                    centroids[c][d] = new_centroids[c][d] / counts[c];
                }
            }
        }
    }

    //build inverted lists
    inverted_lists.assign(k_clusters, std::vector<size_t>());
    for (size_t i = 0; i < db.size(); ++i) {
        inverted_lists[assignments[i]].push_back(i);
    }
}

IVFFlatIndex::IVFFlatIndex(int k_clusters, int nprobe, int dim, unsigned int seed, const std::vector<DatabaseRecord>& database)
    : k_clusters(k_clusters), nprobe(nprobe), dim(dim), db(database) {
    if (this->k_clusters > static_cast<int>(db.size())) {
        this->k_clusters = static_cast<int>(db.size());
    }
    trainKMeans(seed);
}

std::vector<size_t> IVFFlatIndex::queryCandidates(const std::vector<float>& q_vec) const {
    //priority queue to find the nprobe closest centroids (min-heap via pair<distance, centroid_id>)
    std::vector<std::pair<float, int>> centroid_dists;
    centroid_dists.reserve(k_clusters);

    for (int c = 0; c < k_clusters; ++c) {
        float dist = euclideanDistanceSquared(q_vec, centroids[c]);
        centroid_dists.push_back({dist, c});
    }

    int effective_nprobe = std::min(nprobe, k_clusters);
    std::partial_sort(
        centroid_dists.begin(),
        centroid_dists.begin() + effective_nprobe,
        centroid_dists.end()
    );

    std::vector<size_t> candidates;
    for (int i = 0; i < effective_nprobe; ++i) {
        int cluster_id = centroid_dists[i].second;
        const auto& list = inverted_lists[cluster_id];
        candidates.insert(candidates.end(), list.begin(), list.end());
    }

    return candidates;
}

std::vector<SearchResult> RunSearch::runIVFFlatSearch(
    const std::vector<DatabaseRecord>& database,
    const std::vector<SearchQuery>& queries,
    const Parameters parameters
) {
    if (queries.empty() || database.empty()) return {};

    int k_clusters = parameters.ivfflat.kclusters;
    if (k_clusters <= 0) {
        k_clusters = static_cast<int>(std::ceil(std::sqrt(database.size())));
    }

    int nprobe = parameters.ivfflat.nprobe;
    int max_iters = 15;
    unsigned int seed = parameters.seed;

    IVFFlatIndex index(k_clusters, nprobe, max_iters, seed, database);

    std::vector<SearchResult> results;
    results.reserve(queries.size());

    for (const auto& query : queries) {
        auto start = std::chrono::high_resolution_clock::now();

        std::vector<size_t> candidates = index.queryCandidates(query.vector);

        std::priority_queue<std::pair<float, std::string>> max_heap;

        for (size_t idx : candidates) {
            float dist = euclideanDistanceSquared(query.vector, database[idx].vector);
            if (max_heap.size() < 10) {
                max_heap.push({dist, database[idx].image_id});
            } else if (dist < max_heap.top().first) {
                max_heap.pop();
                max_heap.push({dist, database[idx].image_id});
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;

        std::vector<std::string> top10;
        while (!max_heap.empty()) {
            top10.push_back(max_heap.top().second);
            max_heap.pop();
        }
        std::reverse(top10.begin(), top10.end());

        int relevant_at_5 = 0;
        int relevant_at_10 = 0;
        float precision_sum = 0.0f;

        for (size_t rank = 0; rank < top10.size(); ++rank) {
            bool is_relevant = false;
            if (top10[rank].rfind(query.sequence_id, 0) == 0) {
                std::string suffix = top10[rank].substr(query.sequence_id.length());
                if (suffix == "/2" || suffix == "/3" || suffix == "/4" || suffix == "/5" || suffix == "/6") {
                    is_relevant = true;
                }
            }

            if (is_relevant) {
                if (rank < 5) relevant_at_5++;
                if (rank < 10) relevant_at_10++;
                precision_sum += static_cast<float>(relevant_at_10) / static_cast<float>(rank + 1);
            }
        }

        SearchResult res;
        res.sequence_id = query.sequence_id;
        res.timeElapsed = elapsed.count();
        res.top10PhotoIds = top10;
        res.RecallAt5 = relevant_at_5 / 5.0;
        res.RecallAt10 = relevant_at_10 / 5.0;
        res.APAt10 = precision_sum / 5.0;

        results.push_back(res);
    }

    return results;
}