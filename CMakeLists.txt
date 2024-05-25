cmake_minimum_required(VERSION 3.20)
project(chat)

set(CMAKE_CXX_STANDARD 17)

set(CXX_FLAGS
        -ggdb
        -Wall
        -Wextra
        -Werror
        -Wconversion
        -Wno-unused-parameter
        -Wold-style-cast
        -Woverloaded-virtual
        -Wpointer-arith
        -Wshadow
        -Wwrite-strings
	-shared-libasan
        -std=${CMAKE_CXX_STANDARD}
)
string(REPLACE ";" " " CMAKE_CXX_FLAGS "${CXX_FLAGS}")

option(ENABLE_ASAN "Enable AddressSanitizer" OFF)

# Set the CMAKE_CXX_FLAGS variable based on the value of the ENABLE_ASAN option
if (ENABLE_ASAN)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address")
endif ()


set(CMAKE_CXX_FLAGS_DEBUG "-O0")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -DNDEBUG")

# BOOST
find_package(Boost REQUIRED)

# SPDLOG ------------------------
include(FetchContent)

FetchContent_Declare(
        spdlog
        GIT_REPOSITORY https://github.com/gabime/spdlog.git
        GIT_TAG       v1.13.0
)

FetchContent_GetProperties(spdlog)
if (NOT spdlog_POPULATED)
    FetchContent_Populate(spdlog)
    add_subdirectory(${spdlog_SOURCE_DIR} ${spdlog_BINARY_DIR})
endif ()
#------------------------

INCLUDE_DIRECTORIES(SYSTEM ${Boost_INCLUDE_DIR})
INCLUDE_DIRECTORIES(chat_message.h Common.h ChatClient.h ChatServer.h)
add_library(chat Common.cpp ChatClient.cpp ChatServer.cpp)

target_link_libraries(chat PUBLIC -lpthread spdlog::spdlog)
add_executable(client ClientCli.cpp)
add_executable(server ServerCli.cpp)
target_link_libraries(client chat)
target_link_libraries(server chat)

