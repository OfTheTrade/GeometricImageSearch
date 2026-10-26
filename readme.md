# geometricimagesearch
## Compilation/Execution Instructions:
In build/:\
-To compile the main program: Execute "make"\
-To run the main program: Execute "./search -exampleparameter example_value ...". To see needed input consult the bellow paragraph\
-To clean compilation files: Execute "make clean"

In root/:\
-To run cmake: Execute "make" or "make run_cmake"\
-To clean entire build directory: Execute "make clean"

## Parameters
General Parameters (These have no default values and are necessary inputs to run the program):\
-hp <path>: path to the hpatches folder\
-mir <path>: path to the mirflicker folder\
-split <file>: filename of splitfile containing the training, validation, test set split information\
-vocab <int>: the K, ammount of centers of the kmeans\
-S <int>: max ammount of swift descriptors to be extracted of each training image for the kmeans\
-set <validation|test>: the set of the two that will be used in the search\
-o <output file>: file where the output will be placed\
-seed <int>: seed used for reproducability of results where randomness is needed.\
Search Method Specific Parameters (These have default values and are not necessary inputs to run the program):\
-exact OR -lsh OR -hypercube OR -ivfflat OR -ivfpq <search method>: name of the search method to be used\
== Exact:\
-Takes no inputs\
== LSH:\
-k <int>: Ammount of h_i functions making up each fragmentation function g. Default value: 4\
-L <int>: Ammount of fragmentation matrixes. Default value: 5\
-w <double>: Length of the cell on the projection line. Default value: 0.05\
== Hypercube:\
-kproj <int>: Dimension d' of the hypercube. Default value: 10\
-w <double>: Length of the cell on the projection line. Default value: 0.05\
-M <int>: Maximun number of potential points that are checked. Default value: 500\
-probes <int>: Maximun number of vertices of the hypercube that are checked. Default value: 10\
== IVFFlat:\
-kclusters <int>: Number of clusters in the index. Default value: sqrt(n)\
-nprobe <int>: Number of nearest clusters checked during the search. Default value: 5\
== IVFPQ:\
-kclusters <int>: Number of clusters in the index. Default value: sqrt(n)\
-nprobe <int>: Number of nearest clusters checked during the search. Default value: 5\
-M <int>: Number of subimages of product quantization. K must be exactly divisible by M. Default value : 16\
-nbits <int>: Each subspace is clustered into 2^(nbits) clusters. Default Clusters: 256. Default value : 8\
Note: Order of input is not important. If a Search Method Specific Parameter of a search method not used is input, it will be accepted but left unused. General Parameters cannot be input more then once, but Search Method Specific Parameters can, in which case each time it is input it will overwrite it's previous value. Search Method Specific Parameters of the same name share their value across all variants.

## Dependancies:
gxx\
cmake\
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

## Note for Vocabulary Caching
Vocabulary Caching is turned off by default, as to not mess up any automated tests. To enable for the vocabulary, change the global variable "USE_VOCABULART_CACHE" in search.cpp. When the vocabulary training is successfuly run, the results will stored in a file called "vocabulary_cache.xml". If this is turned on, make sure to delete "vocabulary_cache.xml" if any parameters affecting the kmeans, like -K, -S, -seed or the splitfile are changed between executions, or else the results will be incorrect for the parameters given.

## Search Methods
1. Exhaustive Search (-exact)
Description: Sequentially computes the Euclidean distance between the query vector and every vector in the database, returning the 10 nearest neighbors.
Role: Serves as the ground-truth baseline to evaluate retrieval accuracy and runtime performance across all approximate methods.

2. Locality Sensitive Hashing (-lsh)
Parameters: -k <int> (number of hash functions), -L <int> (number of hash tables), -w <double> (projection window width).
Description: Uses $p$-stable distributions to project high-dimensional vectors onto random lines and assigns them into $L$ hash tables. During querying, it exhaustively evaluates only candidate points that fall into the same buckets as the query in at least one table.

3. Random Projection to Hypercube (-hypercube)
Parameters: -kproj <int> (projection dimension $d'$), -w <double> (window width), -M <int> (maximum candidate points to check), -probes <int> (maximum vertices to visit).
Description: Projects database vectors onto the $2^{d'}$ vertices of a binary hypercube. Search starts at the query's projected vertex and traverses neighboring vertices in increasing Hamming distance order, stopping once probes vertices or M candidate points have been inspected.

4. Inverted File Flat (-ivfflat)
Parameters: -kclusters <int> (number of coarse quantization clusters), -nprobe <int> (number of centroids to inspect).
Description: Partitions the database vector space into clusters via k-means and stores records in inverted lists per cluster. At query time, it locates the nprobe nearest cluster centroids and computes exact distances only for the vectors contained within those lists.

5. Inverted File Product Quantization (-ivfpq)
Parameters: -kclusters <int> (number of coarse clusters), -nprobe <int> (clusters to inspect), -M <int> (number of sub-vectors), -nbits <int> (bits per subspace / codebook size $2^{nbits}$).
Description: Applies coarse quantization to the database vectors and computes residual vectors ($r = x - c$). Residuals are split into $M$ sub-vectors and quantized using independent codebooks. Querying across the nprobe nearest coarse clusters is accelerated via Asymmetric Distance Computation (ADC) using precomputed Look-Up Tables (LUT).