cmake_minimum_required(VERSION 2.8.3)
project(mops)

### ROS Dependencies ###
# Find the RTT-ROS package (this transitively includes the Orocos CMake macros)
find_package(catkin REQUIRED COMPONENTS
  rtt_ros
  # ADDITIONAL ROS PACKAGES
  )

include_directories(${catkin_INCLUDE_DIRS} 
  src/Snd 
  src/Rec
  src/MOPS)
### Orocos Dependencies ###
# Note that orocos_use_package() does not need to be called for any dependency
# listed in the package.xml file

include_directories(${USE_OROCOS_INCLUDE_DIRS})

### RTnet 

include_directories(/usr/local/rtnet/include)

### Orocos Targets ###

orocos_component(mops_rec_component src/Rec/mops-rec-component.cpp 
  src/MOPS/MOPS.c
  src/MOPS/MOPS_RTnet_Con.c
  src/MOPS/MQTT.c)

target_link_libraries(mops_rec_component 
  ${catkin_LIBRARIES} 
  ${USE_OROCOS_LIBRARIES} 
  rtdm
  -Wl,@/usr/xenomai/lib/posix.wrappers 
  -L/usr/xenomai/lib
  -lxenomai
  -lpthread_rt
  -lrtdm
  -lrt)
  
#  -Wl,@/usr/xenomai/lib/posix.wrappers -L/usr/xenomai/lib -lpthread_rt -lxenomai -lpthread -lrt -lrtdm
  
orocos_component(mops_snd_component src/Snd/mops-snd-component.cpp 
  src/MOPS/MOPS.c
  src/MOPS/MOPS_RTnet_Con.c
  src/MOPS/MQTT.c)

target_link_libraries(mops_snd_component 
  ${catkin_LIBRARIES} 
  ${USE_OROCOS_LIBRARIES} 
  rtdm
  -Wl,@/usr/xenomai/lib/posix.wrappers 
  -L/usr/xenomai/lib
  -lxenomai
  -lpthread_rt
  -lrtdm
  -lrt)
  
  
orocos_component(mops_broker src/Broker/mops-broker.cpp 
  src/MOPS/MOPS.c
  src/MOPS/MOPS_RTnet_Con.c
  src/MOPS/MQTT.c)

target_link_libraries(mops_broker
  ${catkin_LIBRARIES} 
  ${USE_OROCOS_LIBRARIES} 
  rtdm
  -Wl,@/usr/xenomai/lib/posix.wrappers 
  -L/usr/xenomai/lib
  -lxenomai
  -lpthread_rt
  -lrtdm
  -lrt)
  
# orocos_library(my_library src/my_library.cpp)
# target_link_libraries(my_library ${catkin_LIBRARIES} ${USE_OROCOS_LIBRARIES})

# orocos_service(my_service src/my_service.cpp)
# target_link_libraries(my_service ${catkin_LIBRARIES} ${USE_OROCOS_LIBRARIES})

# orocos_plugin(my_plugin src/my_plugin.cpp)
# target_link_libraries(my_plugin ${catkin_LIBRARIES} ${USE_OROCOS_LIBRARIES})

# orocos_typekit(my_typekit src/my_typekit.cpp)
# target_link_libraries(my_typekit ${catkin_LIBRARIES} ${USE_OROCOS_LIBRARIES})

### Orocos Package Exports and Install Targets ###

# Generate install targets for header files

orocos_install_headers(DIRECTORY include/${PROJECT_NAME})

# Export package information (replaces catkin_package() macro) 
orocos_generate_package(
  INCLUDE_DIRS src/Snd src/Rec src/MOPS
  DEPENDS rtt_ros
)   
