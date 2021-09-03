cmake_minimum_required(VERSION 3.0)


set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "")
set(CMAKE_SUPPRESS_REGENERATION true)

option(BUILD_PANORAMA "Build 360 image processor" ON)
option(BUILD_MAP "Build map processor" ON)
option(BUILD_MODEL "Build 3D model processor" ON)
option(BUILD_CLOUD "Build point cloud processor" ON)


if (BUILD_PANORAMA)
    list(APPEND VCPKG_MANIFEST_FEATURES "panorama")
    message(STATUS "BUILD_PANORAMA ON")
endif()

if (BUILD_MAP)
    list(APPEND VCPKG_MANIFEST_FEATURES "map")
    message(STATUS "BUILD_MAP ON")
endif()

if (BUILD_MODEL)
    list(APPEND VCPKG_MANIFEST_FEATURES "model")
    message(STATUS "BUILD_MODEL ON")
endif()

if (BUILD_CLOUD)
    list(APPEND VCPKG_MANIFEST_FEATURES "cloud")
    message(STATUS "BUILD_CLOUD ON")
endif()

project(voxxlr)

set_property(SOURCE "${CMAKE_SOURCE_DIR}/process.py" PROPERTY VS_DEPLOYMENT_LOCATION "Assets")

if (BUILD_PANORAMA)
    add_subdirectory(src/1d)
endif()

if (BUILD_MAP)
    add_subdirectory(src/2d)
endif()

if (BUILD_MODEL)
    add_subdirectory(src/3d/model)
endif()

if (BUILD_CLOUD)
    add_subdirectory(src/3d/cloud)
endif()

