##*****************************************************************//
##    Albany 2.0:  Copyright 2012 Sandia Corporation               //
##    This Software is released under the BSD license detailed     //
##    in the file "license.txt" in the top-level Albany directory  //
##*****************************************************************//

# SCOREC/meshes/bar ##################

# copy mesh files from current source dir to binary dir
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bar.dmg
               ${CMAKE_CURRENT_BINARY_DIR}/bar.dmg COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bar0.smb
               ${CMAKE_CURRENT_BINARY_DIR}/bar0.smb COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bar1.smb
               ${CMAKE_CURRENT_BINARY_DIR}/bar1.smb COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bar2.smb
               ${CMAKE_CURRENT_BINARY_DIR}/bar2.smb COPYONLY)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/bar3.smb
               ${CMAKE_CURRENT_BINARY_DIR}/bar3.smb COPYONLY)
