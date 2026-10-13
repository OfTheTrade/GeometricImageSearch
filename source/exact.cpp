#include "../include/searchmethod.hpp"
#include <chrono>
#include <queue>
#include <cmath>
#include <unordered_set>

// squared Euclidean distance between two vectors
static inline float distance_sq(const std::vector<float>& a, const std::vector<float>& b) {
    float dist_sq = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        dist_sq += diff * diff;
    }
    return dist_sq;
}
// computes Recall@5, Recall@10, and AP@10 against HPatches ground truth
static void evaluate_result(SearchResult& res){
    // images 2 to 6 are relevant for each sequence,
    // so we create a set of the ground truth image IDs for this sequence
    std::unordered_set<std::string> ground_truth = {
        res.sequence_id + "/2",
        res.sequence_id + "/3",
        res.sequence_id + "/4",
        res.sequence_id + "/5",
        res.sequence_id + "/6"
    };
    int hits_at_5 = 0;
    int hits_at_10 = 0;
    double ap_sum = 0.0;

    for(int rank = 0; rank < static_cast<int>(res.top10PhotoIds.size()); ++rank){
        const std::string& photo_id = res.top10PhotoIds[rank];
        if(ground_truth.find(photo_id) != ground_truth.end()){
            hits_at_10++;
            if(rank < 5){
                hits_at_5++;
            }
            ap_sum += static_cast<double>(hits_at_10) / static_cast<double>(rank + 1);
        }
    }

    res.RecallAt5 = static_cast<double>(hits_at_5) / 5.0;
    res.RecallAt10 = static_cast<double>(hits_at_10) / 5.0;
    res.APAt10 = ap_sum / 5.0;
}

// exact search implementation
std::vector<SearchResult> RunSearch::runExactSearch(
    const std::vector<DatabaseRecord>& database,
    const std::vector<SearchQuery>& queries
) {
    std::vector<SearchResult> all_results;
    all_results.reserve(queries.size());

    for (const auto& query : queries) {
        SearchResult res;
        res.sequence_id = query.sequence_id;
        auto start_time = std::chrono::high_resolution_clock::now();

        // use a max-heap to keep track of the top 10 closest images
        std::priority_queue<std::pair<float, std::string>> max_heap;

        for (const auto& item : database) {
            float dist = distance_sq(query.vector, item.vector);
            if (max_heap.size() < 10) {
                max_heap.push({dist, item.image_id});
            }else if (dist < max_heap.top().first) {
                max_heap.pop();
                max_heap.push({dist, item.image_id});
            }
        }
        
        // extract the top 10 image IDs from the max-heap in ascending distance order
        std::vector<std::string> top10(max_heap.size());
        for (int i = static_cast<int>(max_heap.size()) -1; i>=0; --i) {
            top10[i] = max_heap.top().second;
            max_heap.pop();
        }
        auto end = std::chrono::high_resolution_clock::now();
        res.timeElapsed = std::chrono::duration<double, std::milli>(end - start_time).count();
        res.top10PhotoIds = top10;
        evaluate_result(res);
        all_results.push_back(res);
    }
    return all_results;
}
