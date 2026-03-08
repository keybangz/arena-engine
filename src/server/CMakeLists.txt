add_executable(arena-server
    main.c
)

target_link_libraries(arena-server
    PRIVATE arena-core
)

target_compile_options(arena-server PRIVATE -Wall -Wextra -Wpedantic -g)
