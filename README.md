# geometricimagesearch
## Compilation/Execution Instructions:
In build/:\
-To compile the main program: Execute "make"\
-To run the main program: Execute "./search -exampleparameter example_value ...". To see needed input consult the professor's pdf\
-To clean compilation files: Execute "make clean"

In root/:\
-To run cmake: Execute "make" or "make run_cmake"\
-To clean entire build directory: Execute "make clean"

## Dependancies:
cmake\
gxx\
opencv 

## File Descriptions:
Most header/implementation combos contain one class definition, with one method used to fill that class with data:\
-parameters.hpp/cpp: Used to check the correctness of and store the command line parameters in one class "Parameters". Each variable is afterwards called as field of that class. Needs the argc, argv from main.\
-dataset.hpp/cpp: Used to check the correctness of and store the contents of the splitfile in one class "Dataset".
This class stores the names of each sequence in three seperate structs, according to their set specified in the splitfile.
Needs the path to the splitfile, and the hpatches file (to create the correct filepath it stores for the sequences), both from the "Parameteters" class.\
-vocabulary.hpp/cpp: Used to to compute the centers to be used, storing them in one class "Vocabulary". Uses the sift extraction and kmeans provided by opencv. Needs the "Parameters" (to use the correct parameters) and "Dataset" (to find the training sequences) classes.\
-vectorizer.hpp/cpp: Contains just a static method, used to vectorize any given image against the given centers. Returns the vectorized image as a parameter. Needs the image path, an output vector and the centers (from the vocabulary class).\
-database.hpp/cpp": Used to construct the final database and queries (both stored in the "Database" struct) to be given to the search methods. Needs the "Parameters", "Vocabulary", and "Dataset" classes to properly find the correct set of hpatches images and ammount of mirflicker images to add to the final database.\
-common.hpp: Contains some usefull enums used by other files.\
-search.cpp: The main file, where all the rest are wired together and executed in order: The parameters, the dataset(datasplit), the vocabulary, the database, and the search.\
-searchmethod.hpp: Contains the definitions for the database record and querry structs used by database.hpp/.cpp. Also contains a struct for the results of the search and static method definitions for the different search types.

## Note For Vocabulary Caching
This is turned off by default, as to not mess up any automated tests. To enable, change the global variable "USE_VOCABULART_CACHE" in search.cpp. When the vocabulary training is successfuly run, the results will stored in a file called "vocabulary_cache.xml". If this is turned on, make sure to delete "vocabulary_cache.xml" if any parameters affecting the kmeans, like -K, -S, -seed or the splitfile are changed between executions, or else the results will be incorrect for the parameters given. 
