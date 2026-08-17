cmake_minimum_required(VERSION 3.20)

project(limit_order_book LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(
    ENABLE_SANITIZERS
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

option(
    ENABLE_BENCHMARKS
    "Build performance benchmarks"
    OFF
)

function(enable_sanitizers target)
    if(ENABLE_SANITIZERS)
        if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
            target_compile_options(
                ${target}
                PRIVATE
                    -fsanitize=address,undefined
                    -fno-omit-frame-pointer
                    -g
            )

            target_link_options(
                ${target}
                PRIVATE
                    -fsanitize=address,undefined
                    -fno-omit-frame-pointer
            )
        else()
            message(
                WARNING
                "Sanitizers are only configured for GCC/Clang"
            )
        endif()
    endif()
endfunction()

add_library(lob
    src/order_book.cpp
    src/replay_engine.cpp
    src/event_generator.cpp
)

target_include_directories(lob
    PUBLIC
        ${PROJECT_SOURCE_DIR}/include
)

add_executable(limit_order_book
    src/main.cpp
)

target_link_libraries(limit_order_book
    PRIVATE
        lob
)

include(FetchContent)

FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG v1.17.0
)

FetchContent_MakeAvailable(googletest)

enable_testing()

add_executable(lob_tests
    tests/order_book_test.cpp
    tests/matching_test.cpp
    tests/modify_order_test.cpp
    tests/invariant_test.cpp
    tests/replay_engine_test.cpp
    tests/event_generator_test.cpp
)

target_link_libraries(lob_tests
    PRIVATE
        lob
        GTest::gtest_main
)

include(GoogleTest)

gtest_discover_tests(lob_tests)

enable_sanitizers(lob)
enable_sanitizers(limit_order_book)
enable_sanitizers(lob_tests)

if(ENABLE_BENCHMARKS)
    include(FetchContent)

    set(
        BENCHMARK_ENABLE_TESTING
        OFF
        CACHE BOOL ""
        FORCE
    )

    FetchContent_Declare(
        googlebenchmark
        GIT_REPOSITORY
            https://github.com/google/benchmark.git
        GIT_TAG
            v1.9.5
    )

    FetchContent_MakeAvailable(
        googlebenchmark
    )

    add_executable(
        lob_benchmark
        benchmarks/replay_benchmark.cpp
    )

    target_link_libraries(
        lob_benchmark
        PRIVATE
            lob
            benchmark::benchmark
    )
    add_executable(
        lob_latency
        benchmarks/latency_benchmark.cpp
    )

    target_link_libraries(
        lob_latency
        PRIVATE
            lob
    )
endif()