set(CMAKE_CXX_STANDARD 20)

add_executable(example example.cpp)

target_compile_options(example PRIVATE -Wall -Wfatal-errors -Wextra -Wnon-virtual-dtor -pedantic)

target_link_libraries(example asiofiedpq)
