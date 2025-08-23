# --------------------------------------------------------------------------------------------------
# Deploy Qt for Windows
function(x_deploy_qt_for_windows target)
  get_target_property(QT_TARGET_TYPE Qt${QT_VERSION_MAJOR}::Core TYPE)
  if(QT_TARGET_TYPE STREQUAL "STATIC_LIBRARY")
    return()
  endif()

  if(NOT DEFINED WINDEPLOYQT_EXECUTABLE)
    set(WINDEPLOYQT_EXECUTABLE "${QT_DIR}/../../../bin/windeployqt.exe")
  endif()

  if(NOT EXISTS ${WINDEPLOYQT_EXECUTABLE})
    return()
  endif()

  if(EXISTS ${CMAKE_CURRENT_LIST_DIR}/qml)
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:${target}> --no-compiler-runtime --qmldir
              "${CMAKE_CURRENT_LIST_DIR}/qml"
      COMMENT "Deploy Qt for Windows..."
      VERBATIM)
  else()
    add_custom_command(
      TARGET ${target}
      POST_BUILD
      COMMAND ${WINDEPLOYQT_EXECUTABLE} $<TARGET_FILE:${target}> --no-compiler-runtime
      COMMENT "Deploy Qt for Windows..."
      VERBATIM)
  endif()

  if(MSVC AND NOT ("${CMAKE_BUILD_TYPE}" STREQUAL "Debug"))
    cmake_path(GET CMAKE_CXX_COMPILER PARENT_PATH COMPILER_PATH)
    # add '-' to ignore error if the file does not exist
    add_custom_command(
      TARGET ${target}
      POST_BUILD VERBATIM
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${COMPILER_PATH}/vcruntime140.dll"
              $<TARGET_FILE_DIR:${target}> "||" ${CMAKE_COMMAND} -E true
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${COMPILER_PATH}/vcruntime140_1.dll"
              $<TARGET_FILE_DIR:${target}> "||" ${CMAKE_COMMAND} -E true
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${COMPILER_PATH}/msvcp140.dll"
              $<TARGET_FILE_DIR:${target}> "||" ${CMAKE_COMMAND} -E true
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${COMPILER_PATH}/msvcp140_1.dll"
              $<TARGET_FILE_DIR:${target}> "||" ${CMAKE_COMMAND} -E true
      COMMAND ${CMAKE_COMMAND} -E copy_if_different "${COMPILER_PATH}/msvcp140_2.dll"
              $<TARGET_FILE_DIR:${target}> "||" ${CMAKE_COMMAND} -E true)
    if(EXISTS "${QT_DIR}/../../../bin/libcrypto-3-x64.dll")
      add_custom_command(
        TARGET ${target}
        POST_BUILD VERBATIM
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_DIR}/../../../bin/libcrypto-3-x64.dll"
                $<TARGET_FILE_DIR:${target}>)
    endif()
    if(EXISTS "${QT_DIR}/../../../bin/libssl-3-x64.dll")
      add_custom_command(
        TARGET ${target}
        POST_BUILD VERBATIM
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${QT_DIR}/../../../bin/libssl-3-x64.dll"
                $<TARGET_FILE_DIR:${target}>)
    endif()
  endif()
endfunction()

# --------------------------------------------------------------------------------------------------
# Deploy Qt for macOS
function(x_deploy_qt_for_mac target)
  if(NOT DEFINED MACDEPLOYQT_EXECUTABLE)
    return()
  endif()

  message(STATUS "Target path of macOS is: $<TARGET_FILE_DIR:${target}>")
  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${MACDEPLOYQT_EXECUTABLE} "${target}.app"
    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}
    COMMENT "Making dmg...")
endfunction()

# --------------------------------------------------------------------------------------------------
# Deploy Qt for linux
function(x_deploy_qt_for_linux target)
  # Do nothing...
endfunction()

# --------------------------------------------------------------------------------------------------
# Deploy Qt
function(x_deploy_qt target)
  if(WIN32)
    x_deploy_qt_for_windows(${target})
  elseif(APPLE)
    x_deploy_qt_for_mac(${target})
  elseif(UNIX)
    x_deploy_qt_for_linux(${target})
  endif()
endfunction()
