#include "../include/vocabulary.hpp"

#include <random>

bool Vocabulary::train(const Parameters& parameters, const Dataset& dataset){
    std::vector<float> total_descriptors;
    int total_descriptor_ammount = 0;

    // Seed the rng for cv and std
    cv::setRNGSeed(parameters.seed);
    std::mt19937 rng(parameters.seed);

    cv::Ptr<cv::SIFT> sift = cv::SIFT::create();

    // Go through all hpatches sequences (folders) marked as "train" in the dataset
    for(size_t i = 0; i < dataset.train.size(); i++){
        
        // Go through each photo of the sequence (0 to 6)
        for(int j = 1; j <= 6; j++){

            // Open the image using opencv
            std::string img_path = dataset.train[i].path + "/" + std::to_string(j) + ".ppm";
            cv::Mat img = cv::imread(img_path, cv::IMREAD_GRAYSCALE);

            if (img.empty()){
                std::cerr << "Warning: Unable to read image file: (" << img_path << ")" << std::endl;
                continue;
            }

            // Extract the sift descriptors using opencv
            std::vector<cv::KeyPoint> keypoints;
            cv::Mat raw_descriptors;
            sift->detectAndCompute(img, cv::noArray(), keypoints, raw_descriptors);

            if (raw_descriptors.empty()){
                std::cerr << "Warning: Image with no sift descriptors: (" << img_path << ")" << std::endl;
                continue;
            }

            // Only use up to S descriptors for each image
            int raw_descriptor_ammount = raw_descriptors.rows;

            // Not all raw descriptors fit
            if (raw_descriptor_ammount > parameters.S){
                
                // Holds the new, shuffled index at each (original) index.
                std::vector<int> shuffled_indices(raw_descriptor_ammount);
                for (int k = 0; k < raw_descriptor_ammount; k++){
                    shuffled_indices[k] = k;
                }
                std::shuffle(shuffled_indices.begin(), shuffled_indices.end(), rng);

                // Add S, randomly chosen (seeded) from the entire extracted total, raw descriptors to total_descriptors
                for (int k = 0; k < parameters.S; k++){
                    int shuffled_k = shuffled_indices[k];
                    for (int d = 0; d < 128; d++){
                        total_descriptors.push_back(raw_descriptors.at<float>(shuffled_k,d));
                    }
                }
                total_descriptor_ammount += parameters.S;


            // All raw descriptors fit
            }else{
                
                // Add all raw descriptors to total_descriptors 
                for (int k = 0; k < raw_descriptor_ammount; k++){
                    for (int d = 0; d < 128; d++){
                        total_descriptors.push_back(raw_descriptors.at<float>(k,d));
                    }
                }
                total_descriptor_ammount += raw_descriptor_ammount;
            }

        }

    }

    if (total_descriptor_ammount == 0){
        std::cerr << "Sift extraction resulted in zero total descriptors!" << std::endl;
        return false;
    }
    
    // Copy from vector to the less dynamic format needed by cv::kmeans
    cv::Mat kmeans_descriptors(total_descriptor_ammount, 128, CV_32F);
    for(int i = 0; i < total_descriptor_ammount; i++){
        for(int j = 0; j < 128; j++){
            kmeans_descriptors.at<float>(i,j) = total_descriptors[i * 128 + j];
        }
    }

    // Criteria for kmeans
    cv::TermCriteria criteria(cv::TermCriteria::EPS + cv::TermCriteria::COUNT, 100, 1e-4);

    cv::Mat labels;
    // Execute the kmeans
    try{
        cv::kmeans(kmeans_descriptors, parameters.vocab, labels, criteria, 3, cv::KMEANS_PP_CENTERS, this->centers);
    }catch(cv::Exception& e){
        std::cerr << "Exception during KMeans!" << std::endl;
        return false;
    }

    return !centers.empty();
}