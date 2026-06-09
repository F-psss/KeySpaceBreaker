cmake_minimum_required(VERSION 3.15)

include(FetchContent)
FetchContent_Declare(
        googletest
        URL https://github.com/google/googletest/archive/refs/tags/v1.14.0.zip
        DOWNLOAD_EXTRACT_TIMESTAMP true
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# Один бинарник со всеми тестами — ручными и data-driven
add_executable(decryptor_tests
        tests.cpp
        data_tests.cpp
)
target_link_libraries(decryptor_tests PRIVATE GTest::gtest)
target_include_directories(decryptor_tests PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_definitions(decryptor_tests PRIVATE
        TEST_DATA_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)