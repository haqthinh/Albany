
# 2. Name the test with the directory name
get_filename_component(testName ${CMAKE_CURRENT_SOURCE_DIR} NAME)
# 3. Create the test with this name and standard executable

if (ALBANY_64BIT_INT)
  if (ALBANY_IFPACK2)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/input.xml
               ${CMAKE_CURRENT_BINARY_DIR}/input.xml COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/materials.xml
               ${CMAKE_CURRENT_BINARY_DIR}/materials.xml COPYONLY)
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/turbine.e
               ${CMAKE_CURRENT_BINARY_DIR}/turbine.e COPYONLY)
    add_test(${testName}_Tpetra_64 ${AlbanyT.exe})
  endif ()
endif ()
