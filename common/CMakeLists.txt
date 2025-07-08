add_library(oancommon SHARED
        packet_structs.h
        packet_structs.cpp
        audio_conf.h
        NetworkMapper.cpp
        NetworkMapper.h
        AudioRouter.cpp
        AudioRouter.h
        ClockMaster.cpp
        ClockMaster.h
        clock.h
        ClockSlave.cpp
        ClockSlave.h
)

target_link_libraries(oancommon PUBLIC oannetutils)
target_include_directories(oancommon PUBLIC ${PROJECT_SOURCE_DIR})