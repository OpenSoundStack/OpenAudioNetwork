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
        AudioRouter.cpp
        AudioRouter.h
)

target_link_libraries(oancommon PUBLIC oannetutils)
target_include_directories(oancommon PUBLIC ${PROJECT_SOURCE_DIR})