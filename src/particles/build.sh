#!/bin/bash

if [ -z "$PROJECT_ROOT" ]
then
    echo "PROJECT_ROOT is not set"
    exit 1
fi

export PARTICLE_ROOT="$PROJECT_ROOT/particles" && \

# Generate build script
cd "$PARTICLE_ROOT" && \
if [ ! -d build ]; then
    mkdir build
fi
cd build && \
cmake ../ "-DCMAKE_INSTALL_PREFIX=$PARTICLE_ROOT" && \

# Build and install the program
make -j4 && \
make install && \

# Run the program
cd ../bin && \
./particles
