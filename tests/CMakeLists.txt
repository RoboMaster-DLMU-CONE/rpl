# Include test framework setup
include(CTest)

# Add each test subdirectory
add_subdirectory(serialization)
add_subdirectory(deserialization)
add_subdirectory(parser)
add_subdirectory(integration)
add_subdirectory(traits)