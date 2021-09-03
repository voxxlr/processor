cmake_minimum_required(VERSION 3.0)

project(tiler)

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Boost REQUIRED COMPONENTS log)
find_package(geotiff CONFIG REQUIRED)

file(GLOB TILER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(tiler  ${TILER_FILES})

target_link_libraries(tiler PRIVATE ${PNG_LIBRARY})
target_link_libraries(tiler PRIVATE ${JPEG_LIBRARIES})
target_link_libraries(tiler PRIVATE ZLIB::ZLIB)
target_link_libraries(tiler PRIVATE ${Boost_LIBRARIES})
target_link_libraries(tiler PRIVATE geotiff_library)

target_include_directories(tiler PRIVATE ../common)

target_include_directories(tiler PRIVATE ${PNG_INCLUDE_DIR})
target_include_directories(tiler PRIVATE ${JPEG_INCLUDE_DIR})
target_include_directories(tiler PRIVATE ${Boost_INCLUDE_DIR})

if (WIN32)
    set_target_properties(tiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${CMAKE_SOURCE_DIR}/bin/map" FOLDER "map")
    set_target_properties(tiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${CMAKE_SOURCE_DIR}/bin/map" FOLDER "map")
elseif(UNIX)
    set_target_properties(tiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_SOURCE_DIR}/bin/map" FOLDER "map")
endif()

