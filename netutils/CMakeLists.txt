add_library(oannetutils SHARED
        udp.cpp
        udp.h
        LowLatSocket.cpp
        LowLatSocket.h
)
target_include_directories(oannetutils PUBLIC ${PROJECT_SOURCE_DIR})