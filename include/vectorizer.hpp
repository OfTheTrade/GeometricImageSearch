#ifndef VECTORIZER_H
#define VECTORIZER_H

#include "../include/vocabulary.hpp"

#include <string>
#include <opencv2/opencv.hpp>

class Vectorizer{
    public:
        // Constructs the vectorized version of the image in the given path, according to the given centers
        // Returns true if successful, false in failure
        static bool vectorizeImage(const std::string& img_path, std::vector<float>& img_vector, const Vocabulary& vocabulary);
};

#endif