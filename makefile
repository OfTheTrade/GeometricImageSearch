# Purely here to use the CMakeLists with a smaller command
BUILD_DIR = build
MAIN_EXE = search

.PHONY : run_cmake

run_cmake:
	cmake -S . -B ${BUILD_DIR}
	cmake --build ${BUILD_DIR}

clean:
	rm -rf ${BUILD_DIR}

clear: clean