
.PHONY: all compile build install autoclean clean

all: autoclean

compile:
	cmake -S . -B build $(GENERATOR) -DCMAKE_BUILD_TYPE=Release $(EXT_OPT)

build: compile
	cmake --build build --config Release

install: build
	cmake --install build

autoclean: install
	-cmake -E rm -rf ./.cache
	-cmake -E rm -rf ./build
	-cmake -E rm -rf ./clone

clean:
	-cmake -E rm -rf ./.cache
	-cmake -E rm -rf ./build


