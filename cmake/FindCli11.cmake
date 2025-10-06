find_package(CLI11 CONFIG QUIET)

if (NOT CLI11_FOUND)
    # When building RPLC, disable CLI11 installation
    # CLI11 is header-only and will be compiled into the rplc executable
    set(CLI11_INSTALL OFF CACHE BOOL "Disable CLI11 installation for RPLC" FORCE)
    
    FetchContent_Declare(
            cli11_proj
            QUIET
            GIT_REPOSITORY https://github.com/CLIUtils/CLI11.git
            GIT_TAG v2.5.0
    )
    FetchContent_MakeAvailable(cli11_proj)
endif ()
