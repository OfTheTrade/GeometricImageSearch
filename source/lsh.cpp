#include "../include/lsh.hpp"
#include <cmath>
#include <chrono>
#include <queue>
#include <unordered_set>
#include <algorithm>


// calculate dot product
static inline float dotProduct(const std::vector<float>& a, const std::vector<float>& b) {
    float result = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        result += a[i] * b[i];
    }
    return result;
}

// calculate euclidean distance squared
static inline float euclideanDistanceSquared(const std::vector<float>& a, const std::vector<float>& b) {
    float result = 0.0f;
    for (size_t i = 0; i < a.size(); ++i) {
        float diff = a[i] - b[i];
        result += diff * diff;
    }
    return result;
}

int32_t L2HashFunction::compute(const std::vector<float>& x) const {
    float projection = dotProduct(v, x);
    return static_cast<int32_t>(std::floor((projection+t)/w));
}

uint32_t LSHCompositeFunction::computeID(const std::vector<float>& x) const{
    int64_t sum=0;
    for (size_t i=0; i<h_funcs.size(); ++i){
        int32_t h_val = h_funcs[i].compute(x);
        // modular arithmetic handling negative values safely
        int64_t term= (static_cast<int64_t>(r[i]) * h_val) % M;
        if (term < 0) term += M; // ensure non-negative
        sum = (sum + term) % M;
    }
    return static_cast<uint32_t>(sum);
}

void LSHHashTable::insert(size_t index, const std::vector<float>& vector) {
    uint32_t id = g.computeID(vector);
    buckets[id].push_back(index);
}

const std::vector<size_t>* LSHHashTable::getCandidates(const std::vector<float>& vector, uint32_t& out_id) const {
    out_id = g.computeID(vector);
    auto it = buckets.find(out_id);
    if (it != buckets.end()) {
        return &(it->second);
    } else {
        return nullptr;
    }
}

LSHIndex::LSHIndex(int L, int k, float w, int dim, unsigned int seed, const std::vector<DatabaseRecord>& database)
    : L(L), k(k), w(w), dim(dim), db(database) {
    std::mt19937 rng(seed);
    std::normal_distribution<float> normal_dist(0.0f, 1.0f);
    std::uniform_real_distribution<float> uniform_t(0.0f, w);
    std::uniform_int_distribution<uint32_t> uniform_r(1, 100000000); // Random weights for linear combination

    tables.resize(L);
    for(int l=0; l<L ; ++l){
        tables[l].g.h_funcs.resize(k);
        tables[l].g.r.resize(k);
        for (int j=0; j<k; ++j){
            //generate random weights r_i
            tables[l].g.r[j] = uniform_r(rng);
            // generate projection vector v and offset t
            tables[l].g.h_funcs[j].w = w;
            tables[l].g.h_funcs[j].t = uniform_t(rng);
            tables[l].g.h_funcs[j].v.resize(dim);
            for (int d=0; d<dim; ++d){
                tables[l].g.h_funcs[j].v[d] = normal_dist(rng);
            }
        }
        //insert database items into the hash table l
        for (size_t i=0; i<db.size(); ++i){
            tables[l].insert(i, db[i].vector);
        }
    }
}


std::vector<size_t> LSHIndex::queryCandidates(const std::vector<float>& query_vector) const {
    std::unordered_set<size_t> unique_candidates;
    for (int l = 0; l<L; ++l){
        uint32_t id=0;
        const auto* bucket_ptr = tables[l].getCandidates(query_vector, id);
        if (bucket_ptr != nullptr){
            for(size_t idx : *bucket_ptr){
                unique_candidates.insert(idx);
            }
        }
    }
    return std::vector<size_t>(unique_candidates.begin(), unique_candidates.end());
}

std::vector<SearchResult> RunSearch::runLSHSearch(
    const std::vector<DatabaseRecord>& database,
    const std::vector<SearchQuery>& queries,
    const Parameters parameters)
{
    if(queries.empty() || database.empty()){
        return {};
    }

    int L= parameters.lsh.L;
    int k= parameters.lsh.k;
    float w= parameters.lsh.w;
    unsigned int seed= parameters.seed;
    int dim = database[0].vector.size();

    LSHIndex index(L, k, w, dim, seed, database);

    std::vector<SearchResult> results;
    results.reserve(queries.size());

    for(const auto& query : queries){
        auto start = std::chrono::high_resolution_clock::now();

        // Get candidates from buckets
        std::vector<size_t> candidates = index.queryCandidates(query.vector);

        // max-heap for top 10 closest candidates
        std::priority_queue<std::pair<float, std::string>> max_heap;
        for (size_t idx : candidates){
            float dist = euclideanDistanceSquared(query.vector, database[idx].vector);
            if(max_heap.size() < 10){
                max_heap.push({dist, database[idx].image_id});
            } else if (dist < max_heap.top().first){
                max_heap.pop();
                max_heap.push({dist, database[idx].image_id});
            }
        }
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> elapsed = end - start;

        // export sorted top 10 results
        std::vector<std::string> top10;
        while (!max_heap.empty()){
            top10.push_back(max_heap.top().second);
            max_heap.pop();
        }
        std::reverse(top10.begin(), top10.end());

        // calculate metrics
        int relevant_at_5 = 0;
        int relevant_at_10 = 0;
        double precision_sum = 0.0;

        for (size_t rank=0; rank<top10.size(); ++rank){
            bool is_relevant = false;
            if (top10[rank].rfind(query.sequence_id, 0) == 0){
                std::string suffix = top10[rank].substr(query.sequence_id.length());
                if(suffix == "/2" || suffix == "/3" || suffix == "/4" || suffix == "/5" || suffix == "/6"){
                    is_relevant = true;
                }
            }

            if(is_relevant){
                if(rank < 5) relevant_at_5++;
                if(rank < 10) relevant_at_10++;
                precision_sum += static_cast<double>(relevant_at_10) / (rank + 1);
            }
        }

        SearchResult result;
        result.sequence_id = query.sequence_id;
        result.timeElapsed = elapsed.count();
        result.top10PhotoIds = top10;
        result.RecallAt5= relevant_at_5 / 5.0;
        result.RecallAt10= relevant_at_10 / 5.0;
        result.APAt10= precision_sum / 5.0;

        results.push_back(result);
    }
    return results;
}
