#ifndef SIFTDATA_H
#define SIFTDATA_H

#include <opencv2/opencv.hpp>
#include <vector>

// Contains keypoints and descriptors for a specific database image
struct SiftData{
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    int height;
    int width;
};

#endif