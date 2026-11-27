#include "../include/featurematching.hpp"

#include <cmath>
#include <limits>
#include <utility>
#include <chrono>

// Helper for calculating the euclidian distance between descriptors
double euclideanDistanceDescriptors(const cv::Mat& descriptor_a, const int row_a, const cv::Mat& descriptor_b, const int row_b){
    double sum_sq = 0.0;

    // Emptyness check
    if (descriptor_a.empty() || descriptor_b.empty()) return sum_sq;

    // Pointers to row_a of matrix a and row b of matrix b
    const float* ptr_a = descriptor_a.ptr<float>(row_a);
    const float* ptr_b = descriptor_b.ptr<float>(row_b);

    // Iterate through the colums of the first descriptor
    const int cols = descriptor_a.cols;
    for (int col = 0; col < cols; col++){
        // For each colum
        // Calculate the difference, add it's square to the total sum
        double diff = static_cast<double>(ptr_a[col]) - static_cast<double>(ptr_b[col]);
        sum_sq += diff * diff;
    }

    // Return the root of the total sum
    return std::sqrt(sum_sq);
}

// Helper to perform matching between a query image and a record image
std::vector<PointMatch> matchDescriptors(const SiftData& query_sift_data, const SiftData& record_sift_data, double rho){
    std::vector<PointMatch> matches;

    const cv::Mat& query_descriptors = query_sift_data.descriptors;
    const cv::Mat& record_descriptors = record_sift_data.descriptors;

    // Return empty vector if any of the descriptor matrixes are empty,if the descriptors are of uniqual size, or have less then 2 descriptors
    if (query_descriptors.empty() || record_descriptors.empty() || query_descriptors.cols != record_descriptors.cols || record_descriptors.rows < 2){
        return matches;
    }

    // Record descriptor count and keypoint ammount
    const int query_descriptor_ammount = query_descriptors.rows;
    const int record_descriptor_ammount = record_descriptors.rows;
    const size_t query_keypoint_count = query_sift_data.keypoints.size();
    const size_t record_keypoint_count = record_sift_data.keypoints.size();

    // Avoid execution if sift data is inacurate or somehow mistaken
    if (query_keypoint_count < static_cast<size_t>(query_descriptor_ammount) || record_keypoint_count < static_cast<size_t>(record_descriptor_ammount)){
        return matches;
    }

    matches.reserve(query_descriptor_ammount);

    // Iterate through all descriptors of the query image
    for(int q_descriptor_index = 0; q_descriptor_index < query_descriptor_ammount; q_descriptor_index++){
        // For every query descriptor, track the distance to the closest record descriptor
        double best_distance = std::numeric_limits<double>::infinity();
        // Used for Lowe's check
        double second_best_distance = std::numeric_limits<double>::infinity();
        int best_id = -1;

        // Iterate through all descriptors of the record image
        for(int r_descriptor_index = 0; r_descriptor_index < record_descriptor_ammount; r_descriptor_index++){
            double current_distance = euclideanDistanceDescriptors(query_descriptors, q_descriptor_index, record_descriptors, r_descriptor_index);

            // Skip if the distance is undefined or infinite
            if (!std::isfinite(current_distance)){
                continue;
            }

            // If we found a new best, update the best variables
            if (current_distance < best_distance){
                second_best_distance = best_distance;
                best_distance = current_distance;
                best_id = r_descriptor_index;
            // If instead we found a new second best, update the second best distance
            }else if(current_distance < second_best_distance){
                second_best_distance = current_distance;
            }
        }

        // If there were no valid record keypoints, continue
        if (best_id < 0) continue;

        // Execute the Lowe's ratio procedure to reject some matches
        if (best_distance < rho * second_best_distance){
            // Those that pass the test are added to the final output
            PointMatch point_match;
            point_match.queryPoint = query_sift_data.keypoints[static_cast<size_t>(q_descriptor_index)].pt;
            point_match.candidatePoint = record_sift_data.keypoints[static_cast<size_t>(best_id)].pt;
            matches.push_back(point_match);
        }
    }

    // Final result
    return matches;
    
}

bool FeatureMatcher::runMatching(const std::vector<SearchResult>& searchResults, const Database& database, const Parameters& parameters){
    // Fail if invalid rho is given
    if (parameters.rho <= 0.0 || parameters.rho >= 1.0 || !std::isfinite(parameters.rho)){
        return false;
    }

    std::vector<QueryCorrespondance> temp_output;
    temp_output.reserve(searchResults.size());

    // Go through all search results given
    for (size_t result_index = 0; result_index < searchResults.size(); result_index++){

        // Iterate through searchResults[]
        const SearchResult& current_result = searchResults[result_index];
        SiftData query_sift_data;

        // Fail if a query somehow cannot be found
        if (!database.getQuerySiftDataBySequenceId(current_result.sequence_id, query_sift_data)){
            return false;
        }
        
        // Fail if no descriptors
        if (query_sift_data.descriptors.empty()){
            return false;
        }

        // Start the timer
        auto start_time = std::chrono::high_resolution_clock::now();

        // Prepare the struct to store the matching data of the query image
        size_t current_result_size = current_result.topCPhotoIds.size();

        QueryCorrespondance query_correspondance;
        query_correspondance.sequence_id = current_result.sequence_id;
        query_correspondance.imageCorrespondances.reserve(current_result_size);
    
        for (size_t record_index = 0; record_index < current_result_size; record_index++){

            // Prepare the struct to store the matching data of the record image
            ImageCorrespondance image_corespondance;
            image_corespondance.image_id = current_result.topCPhotoIds[record_index];
            image_corespondance.originalRank = record_index;

            SiftData record_sift_data;
            // If the record is valid, perform the matching
            if (database.getRecordSiftDataByImageId(current_result.topCPhotoIds[record_index], record_sift_data)){
                image_corespondance.pointMatches = matchDescriptors(query_sift_data, record_sift_data, parameters.rho);
            }
            
            // Insert the resulting image corespondance data into the struct of the current querry
            query_correspondance.imageCorrespondances.push_back(std::move(image_corespondance));
        }

        // End the timer and store the time
        auto end_time = std::chrono::high_resolution_clock::now();
        query_correspondance.matchingTimeElapsed = std::chrono::duration<double, std::milli>(end_time - start_time).count();

        // Insert the resulting total query corespondance into temp_output
        temp_output.push_back(query_correspondance);
    }
    
    // When everything is calculated, update the true output
    this->queryCorrespondances = std::move(temp_output);
    return true;
}