cmake_minimum_required(VERSION 3.0.2)
project(gnss_localizer)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

find_package(Eigen3  REQUIRED)

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  roscpp
  std_msgs
  tf
)

# Add GeographicLib.
add_subdirectory(third_party/GeographicLib)



catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES gnss_localizer
#  CATKIN_DEPENDS geometry_msgs roscpp std_msgs tf
#  DEPENDS system_lib
)

###########
## Build ##
###########

include_directories(
  include
  ${catkin_INCLUDE_DIRS}
  ${EIGEN3_INCLUDE_DIR}
  third_party/GeographicLib/include
)

add_executable(gnss_localizer
	src/main.cpp
)

target_link_libraries(gnss_localizer
  ${catkin_LIBRARIES}
	${EIGEN3_LIBS}
  libGeographiccc
)

#############
## Install ##
#############

