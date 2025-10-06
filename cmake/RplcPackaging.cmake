# CPack configuration for RPLC executable package
set(CPACK_PACKAGE_NAME "rplc")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "RPLC - RPL Command Line Tool")

# Generators for executable (end-user tool)
set(CPACK_GENERATOR "TGZ;ZIP;DEB;RPM;WIX")

# Debian settings for executable package
set(CPACK_DEBIAN_PACKAGE_NAME "rplc")
set(CPACK_DEBIAN_PACKAGE_MAINTAINER "${CPACK_PACKAGE_CONTACT}")
set(CPACK_DEBIAN_PACKAGE_SECTION "utils")
set(CPACK_DEBIAN_PACKAGE_DEPENDS "")

# RPM settings for executable package
set(CPACK_RPM_PACKAGE_NAME "rplc")
set(CPACK_RPM_PACKAGE_LICENSE "ISC")
set(CPACK_RPM_PACKAGE_GROUP "Development/Tools")
set(CPACK_RPM_PACKAGE_REQUIRES "")

# WiX installer settings for Windows
set(CPACK_WIX_UPGRADE_GUID "8A5E3B2C-1D4F-4E9A-B8C7-6D5E4F3A2B1C")
set(CPACK_WIX_PROPERTY_ARPCOMMENTS "RPLC - RPL Command Line Tool")
set(CPACK_WIX_PROPERTY_ARPHELPLINK "https://github.com/RoboMaster-DLMU-CONE/rpl")
set(CPACK_WIX_PROPERTY_ARPURLINFOABOUT "https://github.com/RoboMaster-DLMU-CONE/rpl")
set(CPACK_WIX_PROGRAM_MENU_FOLDER "RPLC")
