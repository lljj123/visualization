cmake_minimum_required(VERSION 3.0.2)
project(waypoint_visualization)

find_package(catkin REQUIRED COMPONENTS
  geometry_msgs
  nav_msgs
  rospy
  visualization_msgs
)

catkin_package()

catkin_install_python(PROGRAMS
  scripts/waypoint_marker.py
  DESTINATION ${CATKIN_PACKAGE_BIN_DESTINATION}
)

install(DIRECTORY config launch rviz
  DESTINATION ${CATKIN_PACKAGE_SHARE_DESTINATION}
)
