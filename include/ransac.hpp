#ifndef RANSAC_HPP
#define RANSAC_HPP

#include "../include/verification.hpp"
#include <opencv2/opencv.hpp>
#include <vector>
#include <random>
#include <functional>

// Αποτέλεσμα του RANSAC για μία υποψήφια εικόνα
struct RansacOutput {
    bool valid = false;
    cv::Mat H; // 3x3 πίνακας τύπου CV_64F (double)
    int inliers = 0;
    double medianError = -1.0; // -1.0 αντιστοιχεί σε NA
    std::vector<int> inlierIndices;
};

// Τύπος συνάρτησης για τον υπολογισμό Normalized DLT/SVD (δεν την εχω ακομα)
// Δέχεται >= 4 αντιστοιχίσεις και επιστρέφει true αν παρήχθη έγκυρος 3x3 CV_64F πίνακας H
using HomographyEstimator = std::function<bool(const std::vector<PointMatch>&, cv::Mat&)>;

// Εκτελεί RANSAC + Refit για ένα σύνολο αποδεκτών αντιστοιχίσεων
RansacOutput runRANSAC(
    const std::vector<PointMatch>& matches,
    double tau,
    int T_max,
    double p,
    bool adaptive,
    std::mt19937& rng,
    const HomographyEstimator& estimateHomography
);

#endif // RANSAC_HPP