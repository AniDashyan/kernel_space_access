cmake_minimum_required(VERSION 3.10)
project(kernel_space_access LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)
if(NOT WIN32 AND (CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
    set(ENABLE_ASAN ON)
endif()

add_executable(memory_access main.cpp)

if(WIN32)
    target_compile_definitions(memory_access PRIVATE _CRT_SECURE_NO_WARNINGS)
    if(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
        target_compile_definitions(memory_access PRIVATE USE_SEH)
        target_compile_options(memory_access PRIVATE /EHa)
    endif()
    if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
        target_link_libraries(memory_access PRIVATE kernel32)
    endif()
endif()

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    set(COMPILER_FLAGS -g -O0)
    if(ENABLE_ASAN)
        list(APPEND COMPILER_FLAGS -fsanitize=address)
        target_link_options(memory_access PRIVATE -fsanitize=address)
    endif()
    target_compile_options(memory_access PRIVATE ${COMPILER_FLAGS})
elseif(CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    target_compile_options(memory_access PRIVATE
        /W4
        /Zi
        /EHsc
    )
endif()