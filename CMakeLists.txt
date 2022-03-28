cmake_minimum_required(VERSION 3.14)
project(BPlusTree
    VERSION 1.0.0
    DESCRIPTION "A STL-compliant B+ tree implementation"
    HOMEPAGE_URL "https://github.com/juliencombattelli/bplustree"
)

################################################################################
### Options definition
################################################################################

include(CMakeDependentOption)

option(BPLUSTREE_ENABLE_TEST "Enable building the tests" ON)
cmake_dependent_option(BPLUSTREE_ENABLE_COVERAGE "Enable test coverage report generation" OFF "BPLUSTREE_ENABLE_TEST" OFF)

################################################################################
### Global CMake configuration
################################################################################

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

################################################################################
### Include utility scripts
################################################################################

include(${PROJECT_SOURCE_DIR}/cmake/Scripts/ForceColoredOutput.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Scripts/Sanitizers.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Scripts/StaticAnalyzers.cmake)
include(${PROJECT_SOURCE_DIR}/cmake/Scripts/Warnings.cmake)

################################################################################
### BPlusTree library definition
################################################################################

include(GNUInstallDirs)

add_library(bplustree INTERFACE)
target_include_directories(bplustree
    INTERFACE
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_project_warnings(bplustree)
target_enable_sanitizers(bplustree)

add_library(BPlusTree::BPlusTree ALIAS bplustree)

################################################################################
### Test execution
################################################################################

if(BPLUSTREE_ENABLE_TEST)
    enable_testing()
    include(GoogleTest)
    include(${PROJECT_SOURCE_DIR}/cmake/Scripts/Coverage.cmake)
    add_subdirectory(test)
endif()

################################################################################
### Package config file generation and installation
################################################################################

set(ConfigPackageLocation ${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME})

# Generate CMake package config file
include(CMakePackageConfigHelpers)
configure_package_config_file(
    ${PROJECT_SOURCE_DIR}/cmake/Config/${PROJECT_NAME}Config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/Config/${PROJECT_NAME}Config.cmake
    INSTALL_DESTINATION ${ConfigPackageLocation}
)
write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/cmake/Config/${PROJECT_NAME}ConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

# Install the package config and version files
install(
    FILES
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/Config/${PROJECT_NAME}Config.cmake
        ${CMAKE_CURRENT_BINARY_DIR}/cmake/Config/${PROJECT_NAME}ConfigVersion.cmake
    DESTINATION ${ConfigPackageLocation}
)

################################################################################
### bplustree library installation
################################################################################

# Install the library and the headers, and generate a target file
install(
    TARGETS bplustree
    EXPORT ${PROJECT_NAME}Targets
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/include ${PROJECT_BINARY_DIR}/include
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
    PATTERN "*.in" EXCLUDE
)

# Install the target file
install(
    EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE ${PROJECT_NAME}::
    DESTINATION ${ConfigPackageLocation}
)

# Install the CMake scripts
install(
    DIRECTORY ${PROJECT_SOURCE_DIR}/cmake/Modules
    DESTINATION ${ConfigPackageLocation}
)

################################################################################
### CPack config files generation
################################################################################

include(CPack)
