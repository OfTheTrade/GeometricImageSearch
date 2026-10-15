#ifndef TRAINING_H
#define TRAINING_H

#include "../include/parameters.hpp"
#include "../include/dataset.hpp"

#include <opencv2/opencv.hpp>

// 
class Vocabulary{
    public:
        // The result of the kmeans lies here
        cv::Mat centers;

        // Execute the sift extraction and execute the kmeans
        bool train(const Parameters& parameters, const Dataset& dataset);
};

#endif
