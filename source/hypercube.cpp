#include "../include/hypercube.hpp"
#include <cmath>
#include <chrono>
#include <queue>
#include <algorithm>

static inline float dotProduct(const std::vector<float>& a, const std::vector<float>& b) {
    float result = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        result += a[i] * b[i];
    }
    return result;
}

static inline float euclideanDistance(const std::vector<float>& a, const std::vector<float>& b) {
    float sum = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        sum += diff * diff;
    }
    return sum;
}

void HypercubeHashFunction::init(int dim, float window, unsigned int seed) {
    w = window;
    rng.seed(seed);
    std::normal_distribution<float> normal_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> uniform_t(0.0f, w);
    t = uniform_t(rng);
    v.resize(dim);
    for (int d = 0; d < dim; ++d) {
        v[d] = normal_dist(rng);
    }
}

uint8_t HypercubeHashFunction::computeBit(const std::vector<float>& x) const {
    float proj = dotProduct(v, x);
    int32_t h= static_cast<int32_t>(std::floor((proj + t) / w));
    auto it = f_map.find(h);
    if (it != f_map.end()) {
        return it->second;
    }
    uint8_t bit = static_cast<uint8_t>(bit_dist(rng));
    f_map[h] = bit;
    return bit;
}

HypercubeIndex::HypercubeIndex(int kproj, int M, int probes, float w , int dim, unsigned int seed, const std::vector<DatabaseRecord>& database)
    : kproj(kproj), M(M), probes(probes), w(w), db(database) {
    h_funcs.resize(kproj);
    for (int i = 0; i < kproj; ++i) {
        h_funcs[i].init(dim, w, seed + i * 1377 + 1);
    }
    for (size_t i = 0; i < db.size(); ++i) {
        uint32_t vertex = computeVertex(db[i].vector);
        cube_vertices[vertex].push_back(i);
    }
}

uint32_t HypercubeIndex::computeVertex(const std::vector<float>& x) const {
    uint32_t vertex = 0;
    for (int i = 0; i < kproj; ++i) {
        uint8_t bit = h_funcs[i].computeBit(x);
        vertex |= (static_cast<uint32_t>(bit) << i);
    }
    return vertex;
}

std::vector<size_t> HypercubeIndex::queryCandidates(const std::vector<float>& q_vec) const {
    uint32_t query_vertex = computeVertex(q_vec);
    std::vector<size_t> candidates;
    
    int inspected_vertices = 0;
    int items_checked = 0;

    // search by increasing Hamming distance
    for( int dist=0; dist <= kproj; ++dist){
        if(inspected_vertices >= probes || items_checked >= M) {
            break;
        }

        // traverse all possible masks of Hamming distance 'dist'
        for(uint32_t mask=0; mask < (1u << kproj); ++mask){
            if(__builtin_popcount(mask) == dist) {
                uint32_t current_vertex = query_vertex ^ mask;
                auto it = cube_vertices.find(current_vertex);
                if(it != cube_vertices.end()) {
                    for(size_t idx : it->second) {
                        candidates.push_back(idx);
                        items_checked++;
                        if(items_checked >= M) {
                            break;
                        }
                    }
                }
                inspected_vertices++;
                if(inspected_vertices >= probes || items_checked >= M) {
                    break;
                }
            }
        }
    }
    return candidates;
}

std::vector<SearchResult> RunSearch::runHypercubeSearch(
    const std::vector<DatabaseRecord>& database,
    const std::vector<SearchQuery>& queries,
    const Parameters parameters
) {
    
    if (database.empty() || queries.empty()) {
        return {};
    }

    int dim = database[0].vector.size();
    HypercubeIndex index(
        parameters.hypercube.kproj,
        parameters.hypercube.M,
        parameters.hypercube.probes,
        parameters.hypercube.w,
        dim,
        parameters.seed,
        database
    );

    std::vector<SearchResult> results;
    results.reserve(queries.size());

    for (const auto& query : queries) {
        auto start_time = std::chrono::high_resolution_clock::now();
        std::vector<size_t> candidates = index.queryCandidates(query.vector);
        std::priority_queue<std::pair<float, std::string>, std::vector<std::pair<float, std::string>>, std::greater<std::pair<float, std::string>>> max_heap;
        for (size_t idx : candidates) {
            float dist = euclideanDistance(query.vector, database[idx].vector);
            if(max_heap.size() < 10){
                max_heap.push({dist, database[idx].image_id});
            } else if(dist < max_heap.top().first){
                max_heap.pop();
                max_heap.push({dist, database[idx].image_id});
            }
        }

        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start_time;
        std::vector<std::string> top10;
        while(!max_heap.empty()){
            top10.push_back(max_heap.top().second);
            max_heap.pop();
        }
        std::reverse(top10.begin(), top10.end());
        
        int relevant_at_5 = 0;
        int relevant_at_10 = 0;
        float precision_sum= 0.0f;

        for(size_t rank=0; rank < top10.size(); ++rank){
            bool is_relevant=false;
            if(top10[rank].rfind(query.sequence_id, 0) == 0){
                std::string suffix = top10[rank].substr(query.sequence_id.length());
                if( suffix == "/2" || suffix == "/3" || suffix == "/4" || suffix == "/5" || suffix == "/6"){
                    is_relevant = true;
                }
            }
            if(is_relevant){
                if(rank < 5) relevant_at_5++;
                if(rank < 10) relevant_at_10++;
                precision_sum += static_cast<float>(relevant_at_10) / static_cast<float>(rank + 1);
            }
        }

        SearchResult result;
        result.sequence_id = query.sequence_id;
        result.timeElapsed = elapsed.count();
        result.top10PhotoIds = top10;
        result.RecallAt5 = relevant_at_5 / 5.0;
        result.RecallAt10 = relevant_at_10 / 5.0;
        result.APAt10 = precision_sum / 5.0;

        results.push_back(result);
    }
    return results;
}