cmake_minimum_required(VERSION 3.0)

project(tiler)

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Boost REQUIRED COMPONENTS log)
find_package(TIFF REQUIRED COMPONENTS CXX)


# Try to find GeoTIFF
find_path(GEO_TIFF_INCLUDE_DIR NAMES geotiff.h PATH_SUFFIXES geotiff)
find_library(GEO_TIFF_LIBRARY NAMES geotiff)

if (GEO_TIFF_INCLUDE_DIR AND GEO_TIFF_LIBRARY)
    set(GEO_TIFF_FOUND TRUE)
endif()

if (GEO_TIFF_FOUND)
    set(GEO_TIFF_LIBRARIES ${GEO_TIFF_LIBRARY} ${TIFF_LIBRARIES} ${ZLIB_LIBRARIES})
    set(GEO_TIFF_DEFINITIONS ${TIFF_DEFINITIONS})
endif()

include_directories(${GEO_TIFF_INCLUDE_DIR})


file(GLOB TILER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(tiler  ${TILER_FILES})

target_link_libraries(tiler PRIVATE ${PNG_LIBRARY})
target_link_libraries(tiler PRIVATE ${JPEG_LIBRARIES})
target_link_libraries(tiler PRIVATE ZLIB::ZLIB)
target_link_libraries(tiler PRIVATE ${Boost_LIBRARIES})
target_link_libraries(tiler PRIVATE ${GEO_TIFF_LIBRARIES})

target_include_directories(tiler PRIVATE ../common)

target_include_directories(tiler PRIVATE ${PNG_INCLUDE_DIR})
target_include_directories(tiler PRIVATE ${JPEG_INCLUDE_DIR})
target_include_directories(tiler PRIVATE ${Boost_INCLUDE_DIR})

set_target_properties(tiler PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_SOURCE_DIR}/bin/map" FOLDER "map")

