include(Findcli11)
include(Findfmt)
include(Findjson)

add_executable(rplc
        main.cpp
)

target_link_libraries(rplc PRIVATE CLI11::CLI11 fmt::fmt nlohmann_json::nlohmann_json)