include(FindCli11)
include(Findfmt)

add_executable(rplc
        main.cpp
)

target_link_libraries(rplc PRIVATE CLI11::CLI11 fmt::fmt)