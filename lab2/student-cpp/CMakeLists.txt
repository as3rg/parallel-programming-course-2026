cmake_minimum_required(VERSION 3.20)
project(lab2 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

option(TSAN "Build lab2 with ThreadSanitizer (output: lab2_tsan)" OFF)

add_executable(lab2
    src/main.cpp
    src/scheduler.cpp
)
target_include_directories(lab2 PRIVATE src)
target_link_libraries(lab2 PRIVATE pthread)

if(TSAN)
    if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
        set(TSAN_FLAGS -fsanitize=thread -g -O1)
        target_compile_options(lab2 PRIVATE ${TSAN_FLAGS})
        target_link_options(lab2 PRIVATE ${TSAN_FLAGS})
    else()
        message(FATAL_ERROR "ThreadSanitizer requires Clang or GNU")
    endif()
endif()

add_executable(stress extra/stress.cpp)
target_link_libraries(stress PRIVATE pthread)

add_executable(ministress extra/ministress.cpp)
target_link_libraries(ministress PRIVATE pthread)
