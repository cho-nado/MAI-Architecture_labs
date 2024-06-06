cmake_minimum_required(VERSION 3.2)

SET (SERVICE "delivery")

find_package(OpenSSL)
find_package(Threads)
find_package(Poco REQUIRED COMPONENTS Foundation Util Net XML JSON Crypto NetSSL Data JWT)

if(NOT ${Poco_FOUND})
    message(FATAL_ERROR "Poco C++ Libraries not found.")
endif()

include_directories(${Poco_INCLUDE_DIRS})

SET(CMAKE_CXX_STANDARD 20)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)


include_directories(${PROJECT_SOURCE_DIR})

add_executable(${SERVICE} main.cpp 
                                 config/config.cpp 
                                 database/database.cpp
                                 database/user.cpp)


target_include_directories(${SERVICE} PRIVATE "${CMAKE_BINARY_DIR}")
target_compile_options(${SERVICE} PRIVATE -Wall -Wextra -pedantic -Werror)
set_target_properties(${SERVICE} PROPERTIES LINKER_LANGUAGE CXX)
set_target_properties(${SERVICE} PROPERTIES CXX_STANDARD 20 CXX_STANDARD_REQUIRED ON)

target_link_libraries(${SERVICE} PRIVATE 
                             ${CMAKE_THREAD_LIBS_INIT} 
                             ${Poco_LIBRARIES}
                             "PocoDataPostgreSQL")
