cmake_minimum_required(VERSION 3.14)
project(bestreviewer CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.14.0
)
FetchContent_MakeAvailable(googletest)

include_directories(include)

add_library(tv_controller STATIC src/TVController.cpp)
target_include_directories(tv_controller PUBLIC include)

enable_testing()

option(UPDATE_GOLDEN_MASTER "Rewrite golden expected files from test output" OFF)

add_executable(TunerTest test/TunerTest.cpp)
target_link_libraries(TunerTest GTest::gtest_main GTest::gmock)

add_executable(TVControllerTest test/TVControllerTest.cpp)
target_link_libraries(TVControllerTest tv_controller GTest::gtest_main GTest::gmock)

add_executable(TextTestGoldenTest test/TextTestGoldenTest.cpp)
target_include_directories(TextTestGoldenTest PRIVATE test)
target_link_libraries(TextTestGoldenTest tv_controller GTest::gtest_main)
target_compile_definitions(TextTestGoldenTest PRIVATE
    GOLDEN_EXPECTED_DIR="${CMAKE_SOURCE_DIR}/test/golden"
)
if(UPDATE_GOLDEN_MASTER)
    target_compile_definitions(TextTestGoldenTest PRIVATE GOLDEN_UPDATE_MODE)
endif()

include(GoogleTest)
gtest_discover_tests(TunerTest)
gtest_discover_tests(TVControllerTest)
gtest_discover_tests(TextTestGoldenTest)

# Convenience target: cmake --build build --target update-golden
add_custom_target(update-golden
    COMMAND ${CMAKE_COMMAND} -E env GOLDEN_UPDATE=1
            $<TARGET_FILE:TextTestGoldenTest>
    DEPENDS TextTestGoldenTest
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
    COMMENT "Regenerate test/golden/*.golden.txt from TextTestFixture output"
)
