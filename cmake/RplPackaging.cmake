# CPack configuration for RPL library package
set(CPACK_PACKAGE_NAME "rpl")
set(CPACK_PACKAGE_VERSION "${RPL_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RPL - RoboMaster Packet Library")

# Generators for library (development package)
set(CPACK_GENERATOR "TGZ;ZIP;DEB;RPM")

# Debian settings for development package
set(CPACK_DEBIAN_PACKAGE_NAME "librpl-dev")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "libdevel")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

# RPM settings for development package
set(CPACK_RPM_PACKAGE_NAME "rpl-devel")
set(CPACK_RPM_PACKAGE_LICENSE "ISC")
set(CPACK_RPM_PACKAGE_GROUP "Development/Libraries")
