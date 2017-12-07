#!/bin/bash
# Inspired by https://github.com/google/fruit

set -e

# Helper for travis folding
travis_fold() {
  local action=$1
  local name=$2
  echo -en "travis_fold:${action}:${name}\r"
}

# Helper for distinguishing between different runs
run() {
  case "$1" in
  Build*)
    if [[ "$1" == "Build1" ]]
    then
        # CMake
        travis_fold start cmake
        mkdir build
        cd build
        cmake .. "${CMAKE_ARGS[@]}"
        echo
        if [ -f "CMakeFiles/CMakeError.log" ]
        then
          echo "Content of CMakeFiles/CMakeError.log:"
          cat CMakeFiles/CMakeError.log
        fi
        echo
        cd ..
        travis_fold end cmake
    fi

    # Make
    travis_fold start make
    cd build
    make -j$N_JOBS
    travis_fold end make
    # Set skip-file
    if [[ "$1" != "BuildLast" ]]
    then
        touch skip.txt
    else
        rm -rf skip.txt
    fi
    ;;

  TestAll)
    # Test all
    travis_fold start test_all
    cd build
    # Hack to avoid memout problem with jit and sylvan
    # 1. Run other tests without builder tests
    ctest test --output-on-failure -E run-test-builder
    # 2. Run builder tests without sylvan tests
    ./bin/test-builder --gtest_filter=-"DdJaniModelBuilderTest_Sylvan.*"
    # 3. Just run sylvan tests
    ./bin/test-builder --gtest_filter="DdJaniModelBuilderTest_Sylvan.*"
    travis_fold end test_all
    ;;

  *)
    echo "Unrecognized value of run: $1"
    exit 1
  esac
}


# This only exists in OS X, but it doesn't cause issues in Linux (the dir doesn't exist, so it's
# ignored).
export PATH="/usr/local/opt/coreutils/libexec/gnubin:$PATH"

case $COMPILER in
gcc-6)
    export CC=gcc-6
    export CXX=g++-6
    ;;

gcc)
    export CC=gcc
    export CXX=g++
    ;;

clang-4)
    case "$OS" in
    linux)
        export CC=clang-4.0
        export CXX=clang++-4.0
        ;;
    osx)
        export CC=/usr/local/opt/llvm/bin/clang-4.0
        export CXX=/usr/local/opt/llvm/bin/clang++
        ;;
    *) echo "Error: unexpected OS: $OS"; exit 1 ;;
    esac
    ;;

clang)
    export CC=clang
    export CXX=clang++
    ;;

*)
    echo "Unrecognized value of COMPILER: $COMPILER"
    exit 1
esac

# Build
echo CXX version: $($CXX --version)
echo C++ Standard library location: $(echo '#include <vector>' | $CXX -x c++ -E - | grep 'vector\"' | awk '{print $3}' | sed 's@/vector@@;s@\"@@g' | head -n 1)
echo Normalized C++ Standard library location: $(readlink -f $(echo '#include <vector>' | $CXX -x c++ -E - | grep 'vector\"' | awk '{print $3}' | sed 's@/vector@@;s@\"@@g' | head -n 1))

case "$CONFIG" in
DefaultDebug)           CMAKE_ARGS=(-DCMAKE_BUILD_TYPE=Debug   -DCMAKE_CXX_FLAGS="$STLARG") ;;
DefaultRelease)         CMAKE_ARGS=(-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_FLAGS="$STLARG") ;;
*) echo "Unrecognized value of CONFIG: $CONFIG"; exit 1 ;;
esac

# Restore timestamps of files
travis_fold start mtime
if [[ "$1" == "Build1" ]]
then
    # Remove old mtime cache
    rm -rf travis/mtime_cache/cache.json
fi
ruby travis/mtime_cache/mtime_cache.rb -g travis/mtime_cache/globs.txt -c travis/mtime_cache/cache.json
travis_fold end mtime

run "$1"
