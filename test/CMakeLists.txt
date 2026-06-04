add_executable(oan_uid_autoconf_test
        uid_autoconf_test.cpp
)
target_link_libraries(oan_uid_autoconf_test oancommon)
target_include_directories(oan_uid_autoconf_test PRIVATE ${PROJECT_SOURCE_DIR})

if(COMMAND add_test)
    add_test(NAME oan_uid_autoconf COMMAND oan_uid_autoconf_test)
endif()
