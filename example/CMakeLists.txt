set(CMAKE_CXX_STANDARD 20)

function(add_example EXAMPLE_NAME)
    add_executable(${EXAMPLE_NAME} ${EXAMPLE_NAME}.cpp main.cpp)
    target_link_libraries(${EXAMPLE_NAME} psql)
    target_compile_options(${EXAMPLE_NAME} PRIVATE -Wall -Wfatal-errors -Wextra -pedantic)
endfunction()

add_example(connection_pool)
add_example(notification)
add_example(pipeline)
add_example(prepared_statements)
add_example(simple)
add_example(user_defined)
