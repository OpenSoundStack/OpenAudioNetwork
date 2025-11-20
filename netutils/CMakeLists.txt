set(OAN_UTILS_SOURCES
        udp.cpp
        udp.h
        LowLatSocket.cpp
        LowLatSocket.h
)

if(EMBEDDED_BUILD)
    add_library(oannetutils STATIC ${OAN_UTILS_SOURCES})
else ()
    add_library(oannetutils SHARED ${OAN_UTILS_SOURCES})
endif (EMBEDDED_BUILD)

target_include_directories(oannetutils PUBLIC ${PROJECT_SOURCE_DIR})