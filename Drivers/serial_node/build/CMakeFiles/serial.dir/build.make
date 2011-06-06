# CMAKE generated file: DO NOT EDIT!
# Generated by "Unix Makefiles" Generator, CMake Version 2.8

#=============================================================================
# Special targets provided by cmake.

# Disable implicit rules so canoncical targets will work.
.SUFFIXES:

# Remove some rules from gmake that .SUFFIXES does not remove.
SUFFIXES =

.SUFFIXES: .hpux_make_needs_suffix_list

# Suppress display of executed commands.
$(VERBOSE).SILENT:

# A target that is always out of date.
cmake_force:
.PHONY : cmake_force

#=============================================================================
# Set environment variables for the build.

# The shell in which to execute make rules.
SHELL = /bin/sh

# The CMake executable.
CMAKE_COMMAND = /usr/bin/cmake

# The command to remove a file.
RM = /usr/bin/cmake -E remove -f

# The program to use to edit the cache.
CMAKE_EDIT_COMMAND = /usr/bin/ccmake

# The top-level source directory on which CMake was run.
CMAKE_SOURCE_DIR = /home/jamest/ProjectRinzler/Drivers/serial_node

# The top-level build directory on which CMake was run.
CMAKE_BINARY_DIR = /home/jamest/ProjectRinzler/Drivers/serial_node/build

# Include any dependencies generated for this target.
include CMakeFiles/serial.dir/depend.make

# Include the progress variables for this target.
include CMakeFiles/serial.dir/progress.make

# Include the compile flags for this target's objects.
include CMakeFiles/serial.dir/flags.make

CMakeFiles/serial.dir/src/serial_origin.o: CMakeFiles/serial.dir/flags.make
CMakeFiles/serial.dir/src/serial_origin.o: ../src/serial_origin.cpp
CMakeFiles/serial.dir/src/serial_origin.o: ../manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/ros/tools/rospack/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/ros/core/roslib/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/messages/std_msgs/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/ros/core/rosbuild/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/ros/core/roslang/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/utilities/cpp_common/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/clients/cpp/roscpp_traits/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/utilities/rostime/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/clients/cpp/roscpp_serialization/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/utilities/xmlrpcpp/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/tools/rosconsole/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/messages/rosgraph_msgs/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/clients/cpp/roscpp/manifest.xml
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/messages/std_msgs/msg_gen/generated
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/messages/rosgraph_msgs/msg_gen/generated
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/clients/cpp/roscpp/msg_gen/generated
CMakeFiles/serial.dir/src/serial_origin.o: /opt/ros/diamondback/stacks/ros_comm/clients/cpp/roscpp/srv_gen/generated
	$(CMAKE_COMMAND) -E cmake_progress_report /home/jamest/ProjectRinzler/Drivers/serial_node/build/CMakeFiles $(CMAKE_PROGRESS_1)
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Building CXX object CMakeFiles/serial.dir/src/serial_origin.o"
	/usr/bin/c++   $(CXX_DEFINES) $(CXX_FLAGS) -W -Wall -Wno-unused-parameter -fno-strict-aliasing -pthread -o CMakeFiles/serial.dir/src/serial_origin.o -c /home/jamest/ProjectRinzler/Drivers/serial_node/src/serial_origin.cpp

CMakeFiles/serial.dir/src/serial_origin.i: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Preprocessing CXX source to CMakeFiles/serial.dir/src/serial_origin.i"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -W -Wall -Wno-unused-parameter -fno-strict-aliasing -pthread -E /home/jamest/ProjectRinzler/Drivers/serial_node/src/serial_origin.cpp > CMakeFiles/serial.dir/src/serial_origin.i

CMakeFiles/serial.dir/src/serial_origin.s: cmake_force
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --green "Compiling CXX source to assembly CMakeFiles/serial.dir/src/serial_origin.s"
	/usr/bin/c++  $(CXX_DEFINES) $(CXX_FLAGS) -W -Wall -Wno-unused-parameter -fno-strict-aliasing -pthread -S /home/jamest/ProjectRinzler/Drivers/serial_node/src/serial_origin.cpp -o CMakeFiles/serial.dir/src/serial_origin.s

CMakeFiles/serial.dir/src/serial_origin.o.requires:
.PHONY : CMakeFiles/serial.dir/src/serial_origin.o.requires

CMakeFiles/serial.dir/src/serial_origin.o.provides: CMakeFiles/serial.dir/src/serial_origin.o.requires
	$(MAKE) -f CMakeFiles/serial.dir/build.make CMakeFiles/serial.dir/src/serial_origin.o.provides.build
.PHONY : CMakeFiles/serial.dir/src/serial_origin.o.provides

CMakeFiles/serial.dir/src/serial_origin.o.provides.build: CMakeFiles/serial.dir/src/serial_origin.o
.PHONY : CMakeFiles/serial.dir/src/serial_origin.o.provides.build

# Object files for target serial
serial_OBJECTS = \
"CMakeFiles/serial.dir/src/serial_origin.o"

# External object files for target serial
serial_EXTERNAL_OBJECTS =

../bin/serial: CMakeFiles/serial.dir/src/serial_origin.o
../bin/serial: CMakeFiles/serial.dir/build.make
../bin/serial: CMakeFiles/serial.dir/link.txt
	@$(CMAKE_COMMAND) -E cmake_echo_color --switch=$(COLOR) --red --bold "Linking CXX executable ../bin/serial"
	$(CMAKE_COMMAND) -E cmake_link_script CMakeFiles/serial.dir/link.txt --verbose=$(VERBOSE)

# Rule to build all files generated by this target.
CMakeFiles/serial.dir/build: ../bin/serial
.PHONY : CMakeFiles/serial.dir/build

CMakeFiles/serial.dir/requires: CMakeFiles/serial.dir/src/serial_origin.o.requires
.PHONY : CMakeFiles/serial.dir/requires

CMakeFiles/serial.dir/clean:
	$(CMAKE_COMMAND) -P CMakeFiles/serial.dir/cmake_clean.cmake
.PHONY : CMakeFiles/serial.dir/clean

CMakeFiles/serial.dir/depend:
	cd /home/jamest/ProjectRinzler/Drivers/serial_node/build && $(CMAKE_COMMAND) -E cmake_depends "Unix Makefiles" /home/jamest/ProjectRinzler/Drivers/serial_node /home/jamest/ProjectRinzler/Drivers/serial_node /home/jamest/ProjectRinzler/Drivers/serial_node/build /home/jamest/ProjectRinzler/Drivers/serial_node/build /home/jamest/ProjectRinzler/Drivers/serial_node/build/CMakeFiles/serial.dir/DependInfo.cmake --color=$(COLOR)
.PHONY : CMakeFiles/serial.dir/depend

