add_library(oancommon
        packet_structs.h
        packet_structs.cpp
        audio_conf.h
        NetworkMapper.cpp
        NetworkMapper.h
        AudioPipe.cpp
        AudioPipe.h
        base_pipes/AudioInPipe.cpp
        base_pipes/AudioInPipe.h
        base_pipes/AudioPortalPipe.cpp
        base_pipes/AudioPortalPipe.h
)

target_link_libraries(oancommon PUBLIC oannetutils)
target_include_directories(oancommon PUBLIC ${CMAKE_SOURCE_DIR})