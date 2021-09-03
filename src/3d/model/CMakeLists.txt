cmake_minimum_required(VERSION 3.0)

project(model)

find_package(Boost REQUIRED COMPONENTS log)
find_package(glm CONFIG REQUIRED)

file(GLOB COMMON_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h")
source_group("common" FILES ${COMMON_FILES})

#
# gltf
#

file(GLOB GLTF_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/gltf/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/gltf/*.h")
add_executable(gltf ${GLTF_FILES})

target_link_libraries(gltf PRIVATE ${Boost_LIBRARIES})
target_link_libraries(gltf PRIVATE glm::glm)

target_include_directories(gltf PRIVATE ../../common)
target_include_directories(gltf PRIVATE ${Boost_INCLUDE_DIR})

#
# ifc
#

file(GLOB IFC_FILES "${CMAKE_CURRENT_SOURCE_DIR}/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/*.h" "${CMAKE_CURRENT_SOURCE_DIR}/ifc/*.cc" "${CMAKE_CURRENT_SOURCE_DIR}/ifc/*.h")
add_executable(ifc ${IFC_FILES})

target_link_libraries(ifc PRIVATE ${Boost_LIBRARIES})
target_link_libraries(ifc PRIVATE glm::glm)

target_include_directories(ifc PRIVATE ../../common)
target_include_directories(ifc PRIVATE ${Boost_INCLUDE_DIR})




if (WIN32)
    set_target_properties(ifc gltf PROPERTIES RUNTIME_OUTPUT_DIRECTORY_DEBUG  "${CMAKE_SOURCE_DIR}/bin/model" FOLDER "model")
    set_target_properties(ifc gltf PROPERTIES RUNTIME_OUTPUT_DIRECTORY_RELEASE  "${CMAKE_SOURCE_DIR}/bin/model" FOLDER "model")
    add_definitions(/bigobj)        
elseif(UNIX)
    set_target_properties(ifc gltf PROPERTIES RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}/bin/model" FOLDER "model")
endif()

