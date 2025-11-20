cmake_minimum_required(VERSION 3.20)
project(OpenAudioNetwork)

set(CMAKE_CXX_STANDARD 20)

if(NO_THREADS)
    add_compile_definitions(NO_THREADS)
endif (NO_THREADS)

add_subdirectory(common)
add_subdirectory(netutils)

if(COMPILE_PEER)
    add_subdirectory(peer)
endif (COMPILE_PEER)