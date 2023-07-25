cmake_minimum_required(VERSION 3.8)

project(
  asiofiedpq
  VERSION 0.1.0
  LANGUAGES CXX)

add_library(asiofiedpq INTERFACE)

target_include_directories(asiofiedpq INTERFACE include)
target_include_directories(asiofiedpq INTERFACE third_party)
  
find_package(Boost 1.82 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(asiofiedpq INTERFACE PostgreSQL::PostgreSQL Boost::headers OpenSSL::Crypto)

option(BUILD_EXAMPLES "" ON)

if(BUILD_EXAMPLES)
    add_subdirectory(example)
endif()
