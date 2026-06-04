set(OAN_COMMON_SOURCES
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
        IDiscoverySocket.h
        IUidClock.h
        IUidClock.cpp
        LowLatDiscoverySocket.h
        UidAutoconfigurator.h
        UidAutoconfigurator.cpp
        UidStore.h
        UidStore.cpp
        ../peer/peer_conf.h
)

if(EMBEDDED_BUILD)
    add_library(oancommon STATIC ${OAN_COMMON_SOURCES})
else ()
    add_library(oancommon SHARED ${OAN_COMMON_SOURCES})
endif (EMBEDDED_BUILD)

target_link_libraries(oancommon PUBLIC oannetutils)
target_include_directories(oancommon PUBLIC ${PROJECT_SOURCE_DIR})