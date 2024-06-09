cmake_minimum_required(VERSION 3.0)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)
set_property(GLOBAL PROPERTY PREDEFINED_TARGETS_FOLDER "")
set(CMAKE_SUPPRESS_REGENERATION true)

project(voxxlr)   
 
add_subdirectory(src/1d)
add_subdirectory(src/2d)
add_subdirectory(src/3d/model)
add_subdirectory(src/3d/cloud) 

 