#include "../include/vectorizer.hpp"

bool Vectorizer::vectorizeImage(const std::string& img_path, std::vector<float>& img_vector, const Vocabulary& vocabulary){
    // Open the image using opencv
    cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

    if (img.empty()){
        std::cerr << "Warning: Unable to read image file: (" << img_path << ")" << std::endl;
        return false;
    }

    // Extract the sift descriptors using opencv
    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();
    std::vector<cv::KeyPoint> keypoints;
    cv::Mat descriptors;
    sift->detectAndCompute(img, cv::noArray(), keypoints, descriptors);

    if (descriptors.empty()){
        std::cerr << "Image with no sift descriptors: (" << img_path << ")" << std::endl;
        return false;
    }

    int descriptor_ammount = descriptors.rows;
    int center_ammount = vocabulary.centers.rows;
    std::vector<int> center_descriptor_count(center_ammount, 0);

    // Use opencv matcher with euclidean norm to match each descriptor with it's closest vocabulary center.
    cv::BFMatcher matcher(cv::NORM_L2);
    std::vector<cv::DMatch> matches;
    matcher.match(descriptors, vocabulary.centers, matches);

    // Count the ammount of descriptors each center is closest to
    for (size_t i = 0; i < matches.size(); i++){  
        center_descriptor_count[matches[i].trainIdx]++;
    }

    // Normalise the result, dividing each by the total ammount of descriptors
    img_vector.resize(center_ammount);
    for (int i = 0; i < center_ammount; i++){
        img_vector[i] = static_cast<float>(center_descriptor_count[i]) / static_cast<float>(descriptor_ammount);
    }

    return true;
}