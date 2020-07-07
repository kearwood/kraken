set(triple x86_64-linux-clang)
set(CMAKE_C_COMPILER_TARGET "${LLVM_DEFAULT_TARGET_TRIPLE}" CACHE STRING "")
set(CMAKE_CXX_COMPILER_TARGET "${LLVM_DEFAULT_TARGET_TRIPLE}" CACHE STRING "")

set(CMAKE_C_COMPILER clang)
set(CMAKE_C_COMPILER_TARGET ${triple})
set(CMAKE_CXX_COMPILER clang++)
set(CMAKE_CXX_COMPILER_TARGET ${triple})
set(CMAKE_C_FLAGS "-m64 -DGL_SILENCE_DEPRECATION")
set(CMAKE_CXX_FLAGS "-m64 -std=c++11 -DGL_SILENCE_DEPRECATION")

