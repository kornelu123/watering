all: build ninja

build:
	./build.sh

ninja: build
	ninja -C build/

clean:
	rm -rf build tests/build

test:
	cmake -S tests -B tests/build -GNinja
	cmake --build tests/build
	ctest --test-dir tests/build --output-on-failure

.PHONY: all build ninja clean test
