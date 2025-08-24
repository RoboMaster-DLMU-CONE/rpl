include(Findtl-expected)
include(FindRing-Buffer)
include(Findcppcrc)

add_library(rpl INTERFACE)

target_include_directories(rpl INTERFACE
        ${PROJECT_SOURCE_DIR}/include
        ${ringbuffer_SOURCE_DIR}
)

target_link_libraries(rpl INTERFACE
        cppcrc
        tl::expected
)
