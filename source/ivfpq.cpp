#include "../include/ivfpq.hpp"
#include <cmath>
#include <limits>
#include <random>
#include <algorithm>
#include <chrono>
#include <iostream>

namespace {

// Compute squared Euclidean distance between two vectors
inline float squaredDistance(const float* a, const float* b, int dim) {
    float dist = 0.0f;
    for (int i = 0; i < dim; ++i) {
        float diff = a[i] - b[i];
        dist += diff * diff;
    }
    return dist;
}

// Standard k-means clustering implementation
std::vector<std::vector<float>> runKMeans(
    const std::vector<std::vector<float>>& data,
    int k,
    int max_iters,
    int seed
) {
    int N = static_cast<int>(data.size());
    int dim = static_cast<int>(data[0].size());

    k = std::min(k, N);
    std::vector<std::vector<float>> centroids(k, std::vector<float>(dim, 0.0f));

    std::mt19937 rng(seed);
    std::vector<int> indices(N);
    for (int i = 0; i < N; ++i) indices[i] = i;
    std::shuffle(indices.begin(), indices.end(), rng);

    // Initialize centroids by random sampling
    for (int i = 0; i < k; ++i) {
        centroids[i] = data[indices[i]];
    }

    std::vector<int> assignments(N, 0);

    for (int iter = 0; iter < max_iters; ++iter) {
        bool changed = false;

        // Assignment step: assign each point to nearest centroid
        for (int i = 0; i < N; ++i) {
            float min_d = std::numeric_limits<float>::max();
            int best_c = 0;
            for (int c = 0; c < k; ++c) {
                float d = squaredDistance(data[i].data(), centroids[c].data(), dim);
                if (d < min_d) {
                    min_d = d;
                    best_c = c;
                }
            }
            if (assignments[i] != best_c) {
                assignments[i] = best_c;
                changed = true;
            }
        }

        if (!changed && iter > 0) break;

        // Update step: recompute centroids as mean of assigned points
        std::vector<std::vector<float>> new_centroids(k, std::vector<float>(dim, 0.0f));
        std::vector<int> counts(k, 0);

        for (int i = 0; i < N; ++i) {
            int c = assignments[i];
            counts[c]++;
            for (int d = 0; d < dim; ++d) {
                new_centroids[c][d] += data[i][d];
            }
        }

        for (int c = 0; c < k; ++c) {
            if (counts[c] > 0) {
                for (int d = 0; d < dim; ++d) {
                    centroids[c][d] = new_centroids[c][d] / counts[c];
                }
            } else {
                // Re-initialize empty cluster with a random data point
                centroids[c] = data[rng() % N];
            }
        }
    }

    return centroids;
}

} // namespace

IVFPQIndex::IVFPQIndex(int kclusters, int nprobe_val, int M_val, int nbits_val, int dim)
    : K_dim(dim), k_coarse(kclusters), nprobe(nprobe_val), M(M_val), nbits(nbits_val) {
    d_sub = K_dim / M;
    k_sub = 1 << nbits; // 2^nbits (256 for nbits = 8)
}

void IVFPQIndex::trainAndBuild(const std::vector<DatabaseRecord>& records, int seed) {
    int N = static_cast<int>(records.size());
    if (N == 0) return;

    // 1. Gather database vectors as contiguous floats
    std::vector<std::vector<float>> base_data(N, std::vector<float>(K_dim));
    for (int i = 0; i < N; ++i) {
        for (int d = 0; d < K_dim; ++d) {
            base_data[i][d] = static_cast<float>(records[i].vector[d]);
        }
    }

    // Coarse k-means clustering (k_coarse centroids)
    if (k_coarse <= 0) {
        k_coarse = static_cast<int>(std::ceil(std::sqrt(N)));
    }
    k_coarse = std::min(k_coarse, N);
    coarse_centroids = runKMeans(base_data, k_coarse, 15, seed);
    inverted_lists.resize(k_coarse);

    // 2. Compute residual vectors (r = x - c) and assign to nearest coarse centroid
    std::vector<int> coarse_assignments(N, 0);
    std::vector<std::vector<float>> residuals(N, std::vector<float>(K_dim));

    for (int i = 0; i < N; ++i) {
        float min_d = std::numeric_limits<float>::max();
        int best_c = 0;
        for (int c = 0; c < k_coarse; ++c) {
            float d = squaredDistance(base_data[i].data(), coarse_centroids[c].data(), K_dim);
            if (d < min_d) {
                min_d = d;
                best_c = c;
            }
        }
        coarse_assignments[i] = best_c;
        for (int d = 0; d < K_dim; ++d) {
            residuals[i][d] = base_data[i][d] - coarse_centroids[best_c][d];
        }
    }

    // 3. Train M Product Quantization codebooks on residual sub-vectors
    pq_codebooks.resize(M);

    for (int m = 0; m < M; ++m) {
        int offset = m * d_sub;
        std::vector<std::vector<float>> sub_residuals(N, std::vector<float>(d_sub));
        for (int i = 0; i < N; ++i) {
            for (int d = 0; d < d_sub; ++d) {
                sub_residuals[i][d] = residuals[i][offset + d];
            }
        }
        pq_codebooks[m] = runKMeans(sub_residuals, k_sub, 12, seed + m + 1);
    }

    // 4. Quantize residual vectors and populate the inverted lists
    for (int i = 0; i < N; ++i) {
        QuantizedRecord qrec;
        qrec.image_id = records[i].image_id;
        qrec.codes.resize(M);

        for (int m = 0; m < M; ++m) {
            int offset = m * d_sub;
            float min_d = std::numeric_limits<float>::max();
            uint8_t best_code = 0;

            for (int c = 0; c < k_sub; ++c) {
                float d = squaredDistance(&residuals[i][offset], pq_codebooks[m][c].data(), d_sub);
                if (d < min_d) {
                    min_d = d;
                    best_code = static_cast<uint8_t>(c);
                }
            }
            qrec.codes[m] = best_code;
        }

        int coarse_idx = coarse_assignments[i];
        inverted_lists[coarse_idx].entries.push_back(qrec);
    }
}

std::vector<std::pair<double, std::string>> IVFPQIndex::query(const std::vector<float>& query_vec) const {
    // 1. Find the nprobe nearest coarse centroids to the query vector
    std::vector<std::pair<float, int>> coarse_distances(k_coarse);
    for (int c = 0; c < k_coarse; ++c) {
        float d = squaredDistance(query_vec.data(), coarse_centroids[c].data(), K_dim);
        coarse_distances[c] = {d, c};
    }

    int probes = std::min(nprobe, k_coarse);
    std::partial_sort(coarse_distances.begin(), coarse_distances.begin() + probes, coarse_distances.end());

    std::vector<std::pair<double, std::string>> candidates;

    // 2. Perform Asymmetric Distance Computation (ADC) using Look-Up Tables (LUT)
    for (int p = 0; p < probes; ++p) {
        int coarse_idx = coarse_distances[p].second;
        const auto& inv_list = inverted_lists[coarse_idx];
        if (inv_list.entries.empty()) continue;

        // Query residual: q' = q - c
        std::vector<float> q_residual(K_dim);
        for (int d = 0; d < K_dim; ++d) {
            q_residual[d] = query_vec[d] - coarse_centroids[coarse_idx][d];
        }

        // Precompute LUT: LUT[m][c*] = ||q'_m - codebook[m][c*]||^2
        std::vector<std::vector<float>> lut(M, std::vector<float>(k_sub));
        for (int m = 0; m < M; ++m) {
            int offset = m * d_sub;
            for (int c = 0; c < k_sub; ++c) {
                lut[m][c] = squaredDistance(&q_residual[offset], pq_codebooks[m][c].data(), d_sub);
            }
        }

        // Fast distance evaluation using M table lookups per database point
        for (const auto& entry : inv_list.entries) {
            float approx_dist_sq = 0.0f;
            for (int m = 0; m < M; ++m) {
                approx_dist_sq += lut[m][entry.codes[m]];
            }
            candidates.push_back({static_cast<double>(std::sqrt(approx_dist_sq)), entry.image_id});
        }
    }

    // Sort and select top 10 candidates
    int top_k = std::min(10, static_cast<int>(candidates.size()));
    std::partial_sort(candidates.begin(), candidates.begin() + top_k, candidates.end());
    if (static_cast<int>(candidates.size()) > top_k) {
        candidates.resize(top_k);
    }

    return candidates;
}

std::vector<SearchResult> RunSearch::runIVFPQSearch(
    const std::vector<DatabaseRecord>& records,
    const std::vector<SearchQuery>& queries,
    const Parameters parameters
) {
    std::vector<SearchResult> search_results;
    if (queries.empty() || records.empty()) return search_results;

    int dim = static_cast<int>(records[0].vector.size());
    int kclusters = parameters.ivfpq.kclusters;
    int nprobe = parameters.ivfpq.nprobe;
    int M = parameters.ivfpq.M;
    int nbits = parameters.ivfpq.nbits;
    int seed = parameters.seed;

    // Ensure vocabulary dimension is divisible by M
    if (dim % M != 0) {
        std::cerr << "Error: Vocabulary dimension (" << dim 
                  << ") is not divisible by M (" << M << ")!" << std::endl;
        return search_results;
    }

    // Build the IVF-PQ Index
    IVFPQIndex index(kclusters, nprobe, M, nbits, dim);
    index.trainAndBuild(records, seed);

    search_results.reserve(queries.size());

    for (const auto& query_item : queries) {
        SearchResult res;
        res.sequence_id = query_item.sequence_id;

        std::vector<float> q_vec(dim);
        for (int d = 0; d < dim; ++d) {
            q_vec[d] = static_cast<float>(query_item.vector[d]);
        }

        // Measure query search time in milliseconds (excluding index build)
        auto t_start = std::chrono::high_resolution_clock::now();
        auto top_matches = index.query(q_vec);
        auto t_end = std::chrono::high_resolution_clock::now();

        res.timeElapsed = std::chrono::duration<double, std::milli>(t_end - t_start).count();

        for (const auto& match : top_matches) {
            res.top10PhotoIds.push_back(match.second);
        }

        // Compute Recall@5, Recall@10, and AP@10
        std::vector<int> relevant_ranks;
        int rel_in_top5 = 0;

        for (size_t rank = 0; rank < res.top10PhotoIds.size(); ++rank) {
            const std::string& id = res.top10PhotoIds[rank];
            // An image is relevant if it shares the same sequence_id prefix
            if (id.find(query_item.sequence_id + "/") == 0) {
                relevant_ranks.push_back(static_cast<int>(rank) + 1);
                if (rank < 5) {
                    rel_in_top5++;
                }
            }
        }

        res.RecallAt5 = rel_in_top5 / 5.0;
        res.RecallAt10 = relevant_ranks.size() / 5.0;

        double ap = 0.0;
        for (size_t j = 0; j < relevant_ranks.size(); ++j) {
            ap += (static_cast<double>(j + 1) / relevant_ranks[j]);
        }
        res.APAt10 = ap / 5.0;

        search_results.push_back(res);
    }

    return search_results;
}