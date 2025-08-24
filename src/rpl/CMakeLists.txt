include(Findtl-expected)

add_library(rpl SHARED
        Parser.cpp
)
target_include_directories(rpl PUBLIC
    ${PROJECT_SOURCE_DIR}/include
)

target_link_libraries(rpl PUBLIC
    tl::expected
)