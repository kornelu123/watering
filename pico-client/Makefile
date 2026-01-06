all: build/ ninja

build/:
	./build.sh

ninja: build/
	ninja -C build

clean:
	rm -rf build

flash:
	./rpiflash.sh
