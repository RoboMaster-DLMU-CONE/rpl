find_package(HySerial QUIET)
if (NOT HySerial_FOUND)
    FetchContent_Declare(HySerial
            GIT_REPOSITORY https://github.com/RoboMaster-DLMU-CONE/HySerial.git
            GIT_TAG main
            GIT_SHALLOW true
    )
    FetchContent_MakeAvailable(HySerial)
endif ()