# Purely here to use the CMakeLists with a smaller command
BUILD_DIR = build
MAIN_EXE = main

.PHONY : run_cmake

run_cmake:
	cmake -S . -B ${BUILD_DIR}
	cmake --build ${BUILD_DIR}
	@mkdir -p ${BUILD_DIR}
	@if [ -f ${BUILD_DIR}/${TARGET} ]; then \ cp ${BUILD_DIR}/${TARGET} .; fi

clean:
	rm -rf ${BUILD_DIR}

clear: clean