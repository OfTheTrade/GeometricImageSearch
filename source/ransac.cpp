#include "../include/ransac.hpp"
#include "../include/homography.hpp"
#include <cmath>
#include <algorithm>
#include <limits>
#include <array>

namespace {

// Υπολογισμός διαμέσου (median) ενός μη-κενού vector από double
double computeMedian(std::vector<double>& values) {
    if (values.empty()) return -1.0;
    size_t n = values.size();
    size_t mid = n / 2;
    std::nth_element(values.begin(), values.begin() + mid, values.end());
    double med = values[mid];
    if (n % 2 == 0) {
        double max_left = *std::max_element(values.begin(), values.begin() + mid);
        med = 0.5 * (max_left + med);
    }
    return med;
}

// 2D εξωτερικό γινόμενο (cross product) (B - A) x (C - A) σε ακρίβεια double
inline double cross2D(const cv::Point2d& A, const cv::Point2d& B, const cv::Point2d& C) {
    return (B.x - A.x) * (C.y - A.y) - (B.y - A.y) * (C.x - A.x);
}

// Έλεγχος αν 4 σημεία στην ίδια εικόνα είναι εκφυλισμένα:
// Επαναλαμβανόμενα / σχεδόν ταυτόσημα σημεία
// Συνευθειακές τριάδες (ή σχεδόν συνευθειακές)
// Σχεδόν εκφυλισμένη τετράδα (πολύ μικρό συνολικό εμβαδόν)
bool isDegenerateQuad(const std::array<cv::Point2d, 4>& pts) {
    constexpr double MIN_DIST_SQ = 1e-4;     // ~0.01 pixel ελάχιστη απόσταση σημείων
    constexpr double MIN_TRI_AREA2 = 1.0;    // 2 * εμβαδόν τριγώνου (σε pixels^2)

    // 1. Έλεγχος επαναλαμβανόμενων σημείων
    for (int i = 0; i < 4; ++i) {
        for (int j = i + 1; j < 4; ++j) {
            double dx = pts[i].x - pts[j].x;
            double dy = pts[i].y - pts[j].y;
            if (dx * dx + dy * dy < MIN_DIST_SQ) {
                return true;
            }
        }
    }

    // Έλεγχος και των 4 τριάδων για συνευθειακότητα
    const int triplets[4][3] = {
        {0, 1, 2},
        {0, 1, 3},
        {0, 2, 3},
        {1, 2, 3}
    };

    double max_area2 = 0.0;
    for (int t = 0; t < 4; ++t) {
        double area2 = std::abs(cross2D(
            pts[triplets[t][0]],
            pts[triplets[t][1]],
            pts[triplets[t][2]]
        ));
        if (area2 < MIN_TRI_AREA2) {
            return true; // Συνευθειακή ή σχεδόν συνευθειακή τριάδα
        }
        if (area2 > max_area2) {
            max_area2 = area2;
        }
    }

    // Έλεγχος σχεδόν εκφυλισμένης τετράδας
    if (max_area2 < 2.0 * MIN_TRI_AREA2) {
        return true;
    }

    return false;
}

// Έλεγχος εγκυρότητας 3x3 ομογραφίας (πεπερασμένα στοιχεία & αριθμητική αντιστρεψιμότητα)
bool isValidHomographyMatrix(const cv::Mat& H) {
    if (H.empty() || H.rows != 3 || H.cols != 3 || H.type() != CV_64F) {
        return false;
    }

    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            double val = H.at<double>(r, c);
            if (!std::isfinite(val)) {
                return false;
            }
        }
    }

    // Έλεγχος αντιστρεψιμότητας μέσω ορίζουσας και λόγου ιδιαζουσών τιμών (condition number)
    double det = cv::determinant(H);
    if (!std::isfinite(det) || std::abs(det) < 1e-12) {
        return false;
    }

    cv::SVD svd(H, cv::SVD::NO_UV);
    double s_max = svd.w.at<double>(0, 0);
    double s_min = svd.w.at<double>(2, 0);
    if (s_min < 1e-12 || (s_max / s_min) > 1e8) {
        return false;
    }

    return true;
}

// Αξιολόγηση μιας υπόθεσης H σε όλες τις M αρχικές αντιστοιχίσεις
void evaluateHomography(
    const cv::Mat& H,
    const std::vector<PointMatch>& matches,
    double tau_sq,
    std::vector<int>& out_inlier_indices,
    double& out_median_error
) {
    out_inlier_indices.clear();
    std::vector<double> inlier_errors;
    inlier_errors.reserve(matches.size());

    const double h00 = H.at<double>(0, 0), h01 = H.at<double>(0, 1), h02 = H.at<double>(0, 2);
    const double h10 = H.at<double>(1, 0), h11 = H.at<double>(1, 1), h12 = H.at<double>(1, 2);
    const double h20 = H.at<double>(2, 0), h21 = H.at<double>(2, 1), h22 = H.at<double>(2, 2);

    constexpr double DENOM_EPS = 1e-12;

    for (size_t i = 0; i < matches.size(); ++i) {
        double x = static_cast<double>(matches[i].queryPoint.x);
        double y = static_cast<double>(matches[i].queryPoint.y);

        double a = h00 * x + h01 * y + h02;
        double b = h10 * x + h11 * y + h12;
        double c = h20 * x + h21 * y + h22;

        // Υποχρεωτικός έλεγχος παρονομαστή κατά την προβολή
        if (!std::isfinite(c) || std::abs(c) < DENOM_EPS) {
            continue;
        }

        double u_proj = a / c;
        double v_proj = b / c;

        if (!std::isfinite(u_proj) || !std::isfinite(v_proj)) {
            continue;
        }

        double u_gt = static_cast<double>(matches[i].candidatePoint.x);
        double v_gt = static_cast<double>(matches[i].candidatePoint.y);

        double du = u_gt - u_proj;
        double dv = v_gt - v_proj;
        double err_sq = du * du + dv * dv;

        if (err_sq < tau_sq) {
            out_inlier_indices.push_back(static_cast<int>(i));
            inlier_errors.push_back(std::sqrt(err_sq));
        }
    }

    if (!inlier_errors.empty()) {
        out_median_error = computeMedian(inlier_errors);
    } else {
        out_median_error = std::numeric_limits<double>::infinity();
    }
}

} // namespace

RansacOutput runRANSAC(
    const std::vector<PointMatch>& matches,
    double tau,
    int T_max,
    double p,
    bool adaptive,
    std::mt19937& rng
) {
    RansacOutput result;
    const int M = static_cast<int>(matches.size());

    // Αν υπάρχουν λιγότερες από 4 αντιστοιχίσεις, η επαλήθευση αποτυγχάνει
    if (M < 4 || T_max <= 0) {
        return result;
    }

    const double tau_sq = tau * tau;
    int T_limit = T_max;

    int best_inliers_count = 0;
    double best_median_error = std::numeric_limits<double>::infinity();
    cv::Mat best_H;
    std::vector<int> best_inlier_indices;

    std::uniform_int_distribution<int> dist(0, M - 1);
    std::vector<PointMatch> sample(4);
    std::vector<int> current_inliers;

    for (int iter = 0; iter < T_limit; ++iter) {
        // Τυχαία επιλογή 4 διακριτών δεικτών
        int idx[4];
        for (int k = 0; k < 4; ++k) {
            bool duplicate;
            do {
                duplicate = false;
                idx[k] = dist(rng);
                for (int prev = 0; prev < k; ++prev) {
                    if (idx[k] == idx[prev]) {
                        duplicate = true;
                        break;
                    }
                }
            } while (duplicate);
        }

        std::array<cv::Point2d, 4> q_pts, c_pts;
        for (int k = 0; k < 4; ++k) {
            sample[k] = matches[idx[k]];
            q_pts[k] = cv::Point2d(sample[k].queryPoint.x, sample[k].queryPoint.y);
            c_pts[k] = cv::Point2d(sample[k].candidatePoint.x, sample[k].candidatePoint.y);
        }

        // Υποχρεωτικός έλεγχος εκφυλισμένων τετράδων σε οποιαδήποτε εικόνα
        if (isDegenerateQuad(q_pts) || isDegenerateQuad(c_pts)) {
            continue;
        }

        // Εκτίμηση ομογραφίας H με κανονικοποιημένο DLT/SVD
        cv::Mat H_cand;
        if (!estimateHomographyDLT(sample, H_cand)) {
            continue;
        }
        if (!isValidHomographyMatrix(H_cand)) {
            continue;
        }

        // Υπολογισμός σφαλμάτων επαναπροβολής και inliers
        double current_median = std::numeric_limits<double>::infinity();
        evaluateHomography(H_cand, matches, tau_sq, current_inliers, current_median);
        int current_count = static_cast<int>(current_inliers.size());

        // Επιλογή υπόθεσης με τα περισσότερα inliers και tie-breaker το μικρότερο διάμεσο σφάλμα
        bool is_better = false;
        if (current_count > best_inliers_count) {
            is_better = true;
        } else if (current_count == best_inliers_count && current_count > 0) {
            if (current_median < best_median_error) {
                is_better = true;
            }
        }

        if (is_better) {
            best_inliers_count = current_count;
            best_median_error = current_median;
            best_H = H_cand.clone();
            best_inlier_indices = current_inliers;

            // Προσαρμοζόμενο όριο δοκιμών (adaptive)
            if (adaptive) {
                double w_hat = static_cast<double>(best_inliers_count) / static_cast<double>(M);
                // Ειδικός χειρισμός όταν w_hat == 1 (ή >= 1)
                if (w_hat >= 1.0 - 1e-12) {
                    break;
                }
                // Ειδικός χειρισμός όταν w_hat > 0
                if (w_hat > 0.0) {
                    double w4 = std::pow(w_hat, 4);
                    if (w4 > 0.0 && p > 0.0 && p < 1.0) {
                        double denom = std::log(1.0 - w4);
                        if (denom < 0.0) {
                            double T_new_d = std::ceil(std::log(1.0 - p) / denom);
                            if (std::isfinite(T_new_d) && T_new_d > 0.0) {
                                int T_new = static_cast<int>(std::min<double>(T_max, T_new_d));
                                T_limit = std::min(T_limit, T_new);
                            }
                        }
                    }
                }
            }
        }
    }

    // Αν δεν βρέθηκαν τουλάχιστον 4 inliers, αποτυγχάνει η επαλήθευση
    if (best_inliers_count < 4 || best_H.empty()) {
        return result;
    }

    // Refit, επανεκτίμηση H χρησιμοποιώντας μόνο τα m = |I_best| inliers
    std::vector<PointMatch> inlier_matches;
    inlier_matches.reserve(best_inlier_indices.size());
    for (int i : best_inlier_indices) {
        inlier_matches.push_back(matches[i]);
    }

    cv::Mat H_refit;
    std::vector<int> final_inlier_indices;
    double final_median_error = std::numeric_limits<double>::infinity();

    bool refit_ok = estimateHomographyDLT(inlier_matches, H_refit) && isValidHomographyMatrix(H_refit);

    if (refit_ok) {
        // Επανυπολογισμός inliers σε όλες τις M αρχικές αντιστοιχίσεις με την H_refit
        evaluateHomography(H_refit, matches, tau_sq, final_inlier_indices, final_median_error);
    }

    // Αν το refit παρήγαγε >= 4 έγκυρα τελικά inliers, το κρατάμε· αλλιώς πέφτουμε πίσω στην best_H (αν έχει >= 4)
    if (refit_ok && static_cast<int>(final_inlier_indices.size()) >= 4) {
        result.valid = true;
        result.H = H_refit;
        result.inliers = static_cast<int>(final_inlier_indices.size());
        result.medianError = final_median_error;
        result.inlierIndices = std::move(final_inlier_indices);
    } else {
        // Επαναξιολόγηση της best_H για σιγουριά
        evaluateHomography(best_H, matches, tau_sq, final_inlier_indices, final_median_error);
        if (static_cast<int>(final_inlier_indices.size()) >= 4) {
            result.valid = true;
            result.H = best_H;
            result.inliers = static_cast<int>(final_inlier_indices.size());
            result.medianError = final_median_error;
            result.inlierIndices = std::move(final_inlier_indices);
        }
    }

    return result;
}