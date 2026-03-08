add_executable(asset-packer main.c)
add_executable(entity-editor main.c)

target_compile_options(asset-packer PRIVATE -Wall -Wextra -Wpedantic -g)
target_compile_options(entity-editor PRIVATE -Wall -Wextra -Wpedantic -g)
