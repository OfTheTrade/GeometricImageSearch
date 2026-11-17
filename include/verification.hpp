#ifndef VERIFICATION_H
#define VERIFICATION_H

#include "../include/parameters.hpp"

#include <opencv2/opencv.hpp>

// Struct to hold a single keypoint correspondence between two images
struct PointMatch{
    cv::Point2f queryPoint;
    cv::Point2f candidatePoint;
};

// Struct to contain the correspondance data for a single image
struct ImageCorrespondance{
    // Format "HPatches folder name"/"image number (2-6)" 
    std::string image_id;
    // Rank in the initial search
    int originalRank;
    // Up to M matches of this images keypoints
    std::vector<PointMatch> pointMatches;
};

// Struct to contain the correspondance data of all images of a query/folder, to be given to the RANSAC
struct QueryCorrespondance{
    // HPatches folder
    std::string sequence_id;
    // Time for comparison metrics
    double retrievalTimeElapsed = 0.0;
    double matchingTimeElapsed = 0.0;
    // Up to C candidate images with their point matches
    std::vector<ImageCorrespondance> imageCorrespondances;
};

// The results of the RANSAC and reranking that will be printed out
struct VerificationResults{
    // HPatches folder
    std::string sequence_id;
    int resultsCount;

    // Original top 10 ranked images 
    std::vector<std::string> OriginalTop10PhotoIds;
    // Original Comparison Metrics
    double OriginalRecallAt5 = 0.0;
    double OriginalRecallAt10 = 0.0;
    double OriginalAPAt10 = 0.0;

    // Top 10 ranked images after reranking
    std::vector<std::string> RerankedTop10PhotoIds;
    // Comparison Metrics after reranking
    double RerankedRecallAt5 = 0.0;
    double RerankedRecallAt10 = 0.0;
    double RerankedAPAt10 = 0.0;

    // New comparison metrics
    std::vector<int> RerankedTop10Inliers; 
    std::vector<float> RerankedTop10MedianErrors;

    // Homography metrics
    int validHomographyCount = 0;
    std::vector<double> validPixelErrors; 
    int accurateHomographyCount = 0; // Pixel error < 5 pixels
    
    // Time taken to retrieve the original ranked images
    double retrievalTimeElapsed = 0.0;
    // Time taken for matching, RANSAK and reranking
    double matchingTimeElapsed = 0.0;
    // Total time taken for processing the querry
    double totalTimeElapsed = 0.0;
};

// Execute the verification with a static method
class RunVerification{
    public:
        static std::vector<VerificationResults> runVerification(
                const std::vector<QueryCorrespondance>& QueryCorrespondances,
                const Parameters& parameters
        );
};

#endif