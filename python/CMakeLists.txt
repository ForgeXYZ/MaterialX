# set(SETUP_PY_IN "${CMAKE_CURRENT_SOURCE_DIR}/setup.py.in")
# set(SETUP_PY "${CMAKE_INSTALL_PREFIX}/python/setup.py")

# configure_file(${SETUP_PY_IN} ${SETUP_PY})

# add_custom_command(OUTPUT ${SETUP_PY}
#                    COMMAND ${PYTHON} ${SETUP_PY} build)

if(MATERIALX_INSTALL_PYTHON AND PYTHON_EXECUTABLE)
  install(FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/MaterialX/__init__.py
    ${CMAKE_CURRENT_SOURCE_DIR}/MaterialX/colorspace.py
    ${CMAKE_CURRENT_SOURCE_DIR}/MaterialX/datatype.py
    ${CMAKE_CURRENT_SOURCE_DIR}/MaterialX/main.py
    DESTINATION "${BLENDER_VERSION}/python/lib/python3.7/site-packages/MaterialX"
    )
  # install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MaterialX" DESTINATION "${BLENDER_VERSION}/python/lib/python3.7/site-packages")
  install(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/MaterialXTest" DESTINATION "${BLENDER_VERSION}/lib/MaterialX/test")
  if(MATERIALX_PYTHON_OCIO_DIR)

    if(NOT EXISTS "${MATERIALX_PYTHON_OCIO_DIR}/config.ocio")
      message(WARNING "No file named config.ocio was found in the given OCIO directory.")
    endif()

    install(DIRECTORY "${MATERIALX_PYTHON_OCIO_DIR}/" DESTINATION "lib/MaterialX/config/")
  endif()

endif()
