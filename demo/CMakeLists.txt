add_executable(demo
    main.cpp
)

target_link_libraries(demo PRIVATE
    engine
)

target_compile_options(demo PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -g)
target_link_options(demo PRIVATE -fsanitize=address,undefined)