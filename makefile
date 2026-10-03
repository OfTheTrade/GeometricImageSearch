# Purely here to use the CMakeLists with a smaller command
BUILD_DIR = build
MAIN_EXE = main

.PHONY : run compile clear clean

# To run build/main
run: ${BUILD_DIR}/${MAIN_EXE}
	./${BUILD_DIR}/${MAIN_EXE}

# To compile build/main using cmake generated Makefile
compile: ${BUILD_DIR}/${MAIN_EXE}

# To generate Makefile using cmake
run_cmake: ${BUILD_DIR}/CMakeCache.txt

${BUILD_DIR}/${MAIN_EXE}: ${BUILD_DIR}/CMakeCache.txt
	cmake --build ${BUILD_DIR}

${BUILD_DIR}/CMakeCache.txt: CMakeLists.txt
	cmake -S . -B ${BUILD_DIR}

clean:
	rm -rf ${BUILD_DIR}

clear: clean