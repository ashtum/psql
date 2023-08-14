cmake_minimum_required(VERSION 3.8)

project(
  psql
  VERSION 0.1.0
  LANGUAGES CXX)

add_library(psql INTERFACE)

target_include_directories(psql INTERFACE include)
  
find_package(Boost 1.82 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(psql INTERFACE PostgreSQL::PostgreSQL Boost::headers OpenSSL::Crypto)

option(BUILD_EXAMPLES "" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
