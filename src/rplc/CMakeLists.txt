include(FindCli11)
include(FindFmt)
include(FindJson)

add_executable(rplc
        main.cpp
        src/Config.cpp
        src/Validator.cpp
        src/Generator.cpp
        src/FileWriter.cpp
        src/CLI.cpp
)

target_include_directories(rplc PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(rplc PRIVATE CLI11::CLI11 fmt::fmt nlohmann_json::nlohmann_json)