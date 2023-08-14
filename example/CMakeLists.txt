set(CMAKE_CXX_STANDARD 20)

add_executable(simple main.cpp simple.cpp)
add_executable(pipeline main.cpp pipeline.cpp)
add_executable(user_defined main.cpp user_defined.cpp)

target_compile_options(simple PRIVATE -Wall -Wfatal-errors -Wextra -Wnon-virtual-dtor -pedantic)
target_compile_options(pipeline PRIVATE -Wall -Wfatal-errors -Wextra -Wnon-virtual-dtor -pedantic)
target_compile_options(user_defined PRIVATE -Wall -Wfatal-errors -Wextra -Wnon-virtual-dtor -pedantic)

target_link_libraries(simple psql)
target_link_libraries(pipeline psql)
target_link_libraries(user_defined psql)
