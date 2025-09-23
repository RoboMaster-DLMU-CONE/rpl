include(FindCli11)
include(FindFmt)
include(FindJson)

add_executable(rplc
        main.cpp
)

target_link_libraries(rplc PRIVATE CLI11::CLI11 fmt::fmt nlohmann_json::nlohmann_json)