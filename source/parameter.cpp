#include "../include/parameters.hpp"
#include <cstring>

// Parses args and fills out the parameter struct
// True on success, False on failure
bool Parameters::parseArgv(int argc, char* argv[]){
    // Stores the ammount of times a parameter has been set
    int basicParameterExists[10] = {0};

    // Go through argv, accepting correctly formated parameters
    // If it reaches something it does not recognise, return with false
    for (int i = 1; i < argc; ++i){
        if(std::strcmp(argv[i],"-hp") == 0 && i + 1 < argc){
            this->hp_path = argv[++i];
            basicParameterExists[0]++;

        }else if(std::strcmp(argv[i],"-mir") == 0 && i + 1 < argc){
            this->mir_path = argv[++i];
            basicParameterExists[1]++;

        }else if(std::strcmp(argv[i],"-split") == 0 && i + 1 < argc){
            this->split_path = argv[++i];
            basicParameterExists[2]++;

        }else if(std::strcmp(argv[i],"-vocab") == 0 && i + 1 < argc){
            this->vocab = atoi(argv[++i]);
            basicParameterExists[3]++;

        }else if(std::strcmp(argv[i],"-S") == 0 && i + 1 < argc){
            this->S = atoi(argv[++i]);
            basicParameterExists[4]++;

        }else if(std::strcmp(argv[i],"-D") == 0 && i + 1 < argc){
            this->D = atoi(argv[++i]);
            basicParameterExists[5]++;

        }else if(std::strcmp(argv[i],"-set") == 0 && i + 1 < argc && std::strcmp(argv[i+1],"validation") == 0){
            i++;
            this->set = Set::VALIDATION;
            basicParameterExists[6]++;

        }else if(std::strcmp(argv[i],"-set") == 0 && i + 1 < argc && std::strcmp(argv[i+1],"test") == 0){
            i++;
            this->set = Set::TEST;
            basicParameterExists[6]++;

        }else if(std::strcmp(argv[i],"-o") == 0 && i + 1 < argc){
            this->output_path = argv[++i]; 
            basicParameterExists[7]++;

        }else if(std::strcmp(argv[i],"-seed") == 0 && i + 1 < argc){
            this->seed = atoi(argv[++i]);
            basicParameterExists[8]++;


        }else if(std::strcmp(argv[i],"-exact") == 0){
            this->searchMethod = SearchMethod::EXACT;
            basicParameterExists[9]++;

        }else if(std::strcmp(argv[i],"-lsh") == 0){
            this->searchMethod = SearchMethod::LSH;
            basicParameterExists[9]++;

        }else if(std::strcmp(argv[i],"-hypercube") == 0){
            this->searchMethod = SearchMethod::HYPERCUBE;
            basicParameterExists[9]++;

        }else if(std::strcmp(argv[i],"-ivfflat") == 0){
            this->searchMethod = SearchMethod::IVFFLAT;
            basicParameterExists[9]++;

        }else if(std::strcmp(argv[i],"-ivfpq") == 0){
            this->searchMethod = SearchMethod::IVFPQ;
            basicParameterExists[9]++;


        }else if(std::strcmp(argv[i],"-k") == 0 && i + 1 < argc){
            this->lsh.k = atoi(argv[++i]);

        }else if(std::strcmp(argv[i],"-L") == 0 && i + 1 < argc){
            this->lsh.L = atoi(argv[++i]);

        }else if(std::strcmp(argv[i],"-w") == 0 && i + 1 < argc){
            float temp = atof(argv[++i]);
            this->lsh.w = temp;
            this->hypercube.w = temp;


        }else if(std::strcmp(argv[i],"-kproj") == 0 && i + 1 < argc){
            this->hypercube.kproj = atoi(argv[++i]);

        }else if(std::strcmp(argv[i],"-M") == 0 && i + 1 < argc){
            int temp = atoi(argv[++i]);
            this->hypercube.M = temp;
            this->ivfpq.M = temp;

        }else if(std::strcmp(argv[i],"-probes") == 0 && i + 1 < argc){
            this->hypercube.probes = atoi(argv[++i]);

        
        }else if(std::strcmp(argv[i],"-kclusters") == 0 && i + 1 < argc){
            int temp = atoi(argv[++i]);
            this->ivfflat.kclusters = temp;
            this->ivfpq.kclusters = temp;

        }else if(std::strcmp(argv[i],"-nprobe") == 0 && i + 1 < argc){
            int temp = atoi(argv[++i]);
            this->ivfflat.nprobe = temp;
            this->ivfpq.nprobe = temp;

        }else if(std::strcmp(argv[i],"-nbits") == 0 && i + 1 < argc){
            this->ivfpq.nbits = atoi(argv[++i]);

        }else{
            printf("Malformed parameter input, call in any order as specified in the pdf!\n");
            return false;
        }
    }

    // Detect missing or redundant crucial parameters
    for (int i = 0; i < 10; i++){
        if (basicParameterExists[i] != 1){
            printf("Missing or redundant parameter input, call in any order as specified in the pdf!\n");
            return false;
        }
    }

    return true;
}
