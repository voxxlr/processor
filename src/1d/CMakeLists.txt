cmake_minimum_required(VERSION 3.0)

project(panorama)

find_package(PNG REQUIRED)
find_package(JPEG REQUIRED)
find_package(Boost REQUIRED COMPONENTS log)

file(GLOB CUBER_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
add_executable(cuber  ${CUBER_FILES})

target_link_libraries(cuber PRIVATE ${PNG_LIBRARY})
target_link_libraries(cuber PRIVATE ${JPEG_LIBRARIES})
target_link_libraries(cuber PRIVATE ZLIB::ZLIB)
target_link_libraries(cuber PRIVATE ${Boost_LIBRARIES})

target_include_directories(cuber PRIVATE ../common)

target_include_directories(cuber PRIVATE ${PNG_INCLUDE_DIR})
target_include_directories(cuber PRIVATE ${JPEG_INCLUDE_DIR})
target_include_directories(cuber PRIVATE ${Boost_INCLUDE_DIR})

if (WIN32)
    set_target_properties(cuber PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${CMAKE_SOURCE_DIR}/bin/panorama" FOLDER "panorama")
    set_target_properties(cuber PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${CMAKE_SOURCE_DIR}/bin/panorama" FOLDER "panorama")
elseif(UNIX)
    set_target_properties(cuber PROPERTIES RUNTIME_OUTPUT_DIRECTORY  "${CMAKE_SOURCE_DIR}/bin/panorama" FOLDER "panorama")
endif()

