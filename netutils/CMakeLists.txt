set(OAN_UTILS_SOURCES
        udp.cpp
        udp.h
        LowLatSocket.h
        rt.h
        rt.cpp
        platforms/lls_linux.h
        lls_common.h
        platforms/lls_linux.cpp
        platforms/lls_zephyr.h
        platforms/lls_zephyr.cpp
)

if(EMBEDDED_BUILD)
    add_library(oannetutils STATIC ${OAN_UTILS_SOURCES})
else ()
    add_library(oannetutils SHARED ${OAN_UTILS_SOURCES})
endif (EMBEDDED_BUILD)

target_include_directories(oannetutils PUBLIC ${PROJECT_SOURCE_DIR})