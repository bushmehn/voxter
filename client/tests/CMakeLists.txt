set(TEST_SOURCES
    tst_appstore.cpp
    ../src/store/AppStore.cpp
    ../src/store/AppStore.h
)

qt_add_executable(voxter_tests
    ${TEST_SOURCES}
)

target_link_libraries(voxter_tests PRIVATE
    Qt6::Core
    Qt6::Test
)

target_include_directories(voxter_tests PRIVATE ../src)

add_test(NAME voxter_tests COMMAND voxter_tests)
