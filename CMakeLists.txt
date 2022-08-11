CMAKE_MINIMUM_REQUIRED(VERSION 3.0.0)


set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} --std=c++17 -O3")

set(PROJECT_NAME
    vmprobe
)
set(LIBRARIES_DIR
    libs
)
set(LIBRARIES 
)

file(  
  GLOB_RECURSE
  PROJECT_FILES
  src/*.cpp
  )

project(${PROJECT_NAME})

###########
# Project #
###########
add_executable(${PROJECT_NAME} ${PROJECT_FILES})

foreach(LIBRARY ${LIBRARIES})
    add_subdirectory("${LIBRARIES_DIR}/${LIBRARY}")
endforeach(LIBRARY)

target_link_libraries(${PROJECT_NAME} -lvirt ${LIBRARIES})