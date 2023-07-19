cmake_minimum_required(VERSION 3.8)

project(
  asiofied_libpq
  VERSION 0.1.0
  LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)

add_executable(main src/main.cpp)

target_compile_options(main PRIVATE -Wall -Wfatal-errors -Wextra -Wnon-virtual-dtor -pedantic)

find_package(Boost 1.82 REQUIRED)
find_package(PostgreSQL REQUIRED)
find_package(OpenSSL REQUIRED)

target_link_libraries(main PostgreSQL::PostgreSQL Boost::headers OpenSSL::Crypto)

target_include_directories(main PUBLIC third_party)
