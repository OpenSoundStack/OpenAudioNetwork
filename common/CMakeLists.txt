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
        base_pipes/AudioPortalRxPipe.cpp
        base_pipes/AudioPortalRxPipe.h
        base_pipes/LevelMeasurePipe.cpp
        base_pipes/LevelMeasurePipe.h
        AudioRouter.cpp
        AudioRouter.h
)

target_link_libraries(oancommon PUBLIC oannetutils)
target_include_directories(oancommon PUBLIC ${PROJECT_SOURCE_DIR})