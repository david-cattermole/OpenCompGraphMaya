# Create Python file with some build options baked in.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenCompGraphMaya/constant.py.in
  ${CMAKE_CURRENT_SOURCE_DIR}/OpenCompGraphMaya/constant.py
  @ONLY)

install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/"
  DESTINATION "${MODULE_FULL_NAME}/python"
  FILES_MATCHING PATTERN "*.py")
