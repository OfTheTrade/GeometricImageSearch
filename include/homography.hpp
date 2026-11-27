#ifndef HOMOGRAPHY_H
#define HOMOGRAPHY_H

#include "../include/verification.hpp"

#include <opencv2/opencv.hpp>

// Estimates the 3 by 3 homography matrix
// Returns true if H is successfuly calculated and valid, false otherwise
bool estimateHomographyDLT(const std::vector<PointMatch>& pointMatches, cv::Mat& H);

#endif