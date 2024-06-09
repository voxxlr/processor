cmake_minimum_required(VERSION 3.0)

project(cloud)

find_package(Boost REQUIRED COMPONENTS log)
find_package(glm CONFIG REQUIRED)
find_package(XercesC)

include_directories(PRIVATE ../../common)
include_directories(PRIVATE ${Boost_INCLUDE_DIR})

file(GLOB COMMON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
source_group("common" FILES ${COMMON_FILES})

#
# analyzer
#
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/analyzer/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/analyzer/*.h")
add_executable(analyzer ${SOURCE_FILES})
target_link_libraries(analyzer PRIVATE ${Boost_LIBRARIES})
set_property(TARGET analyzer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/process)

#
# filter
#
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/filter/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/filter/*.h")
add_executable(filter ${SOURCE_FILES})
target_link_libraries(filter PRIVATE ${Boost_LIBRARIES})
set_property(TARGET filter PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/process)

#
# Packetizer
#
file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/packetizer/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/packetizer/*.h")
add_executable(packetizer ${SOURCE_FILES})
target_link_libraries(packetizer PRIVATE ${Boost_LIBRARIES})
set_property(TARGET packetizer PROPERTY VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/process)

#
# Importer
#
if (UNIX)
    add_compile_definitions(LINUX)
endif()

file(GLOB SOURCE_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/importer/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/importer/*.h")
file(GLOB FORMAT_FILES "${CMAKE_CURRENT_SOURCE_DIR}/importer/formats/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/importer/formats/*.h")
file(GLOB E57_FILES "${CMAKE_CURRENT_SOURCE_DIR}/importer/formats/e57/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/importer/formats/e57/*.h")
list (APPEND SOURCE_FILES ${FORMAT_FILES})
list (APPEND SOURCE_FILES ${E57_FILES})
add_executable(importer ${SOURCE_FILES})

source_group("formats" FILES ${FORMAT_FILES})
source_group("formats/e57" FILES ${E57_FILES})

find_path(LASZIP_INCLUDE_DIR laszip/laszip_api.h)
target_include_directories(importer PRIVATE ${LASZIP_INCLUDE_DIR})

target_link_libraries(importer PRIVATE ${Boost_LIBRARIES})
target_link_libraries(importer PRIVATE ${XercesC_LIBRARIES})

find_library(LASZIP_LIBRARY laszip)

target_link_libraries(importer PRIVATE ${LASZIP_LIBRARY})

set_target_properties(importer analyzer filter packetizer PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/cloud" FOLDER "cloud")
