include(FindTl-expected)
include(FindCppcrc)
include(FindFrozen)

add_library(rpl INTERFACE)

target_include_directories(rpl INTERFACE
        ${PROJECT_SOURCE_DIR}/include
        ${ringbuffer_SOURCE_DIR}
)

target_link_libraries(rpl INTERFACE
        cppcrc
        tl::expected
        frozen
)

target_compile_definitions(rpl INTERFACE
        FRAME_HEADER_TAG=${FRAME_HEADER_TAG}
)