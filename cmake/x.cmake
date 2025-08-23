# --------------------------------------------------------------------------------------------------
# Get the last tag.
function(x_git_get_latest_tag working_dir prefix)
  execute_process(
    COMMAND git describe --abbrev=0 --tags
    WORKING_DIRECTORY ${working_dir}
    OUTPUT_VARIABLE GIT_LATEST_TAG
    OUTPUT_STRIP_TRAILING_WHITESPACE)

  set(USING_DATE_TIME FALSE)
  if(NOT GIT_LATEST_TAG)
    set(USING_DATE_TIME TRUE)
  else()
    if(${GIT_LATEST_TAG} STREQUAL "")
      set(USING_DATE_TIME TRUE)
    elseif(${GIT_LATEST_TAG} STREQUAL "continuous")
      set(USING_DATE_TIME TRUE)
    endif()
  endif()

  if(USING_DATE_TIME)
    string(TIMESTAMP current_year "%Y")
    string(TIMESTAMP current_month "%m")
    string(TIMESTAMP current_day "%d")
    math(EXPR current_month "${current_month} + 0")
    math(EXPR current_day "${current_day} + 0")
    set(target_version "v${current_year}.${current_month}.${current_day}")
    set(GIT_LATEST_TAG ${target_version})
  endif()

  message(STATUS "[git] Latest git tag(${prefix}_LATEST_GIT_TAG): ${GIT_LATEST_TAG}")
  set(${prefix}_LATEST_GIT_TAG="${GIT_LATEST_TAG}" CACHE STRING "Latest git tag" FORCE)
  set(${prefix}_LATEST_GIT_TAG
      "${GIT_LATEST_TAG}"
      PARENT_SCOPE)
  add_compile_definitions(${prefix}_LATEST_GIT_TAG="${GIT_LATEST_TAG}")
endfunction()

# --------------------------------------------------------------------------------------------------
# Get the last commit.
function(x_git_get_latest_commit working_dir prefix)
  execute_process(
    COMMAND git log -1 --pretty=%H
    WORKING_DIRECTORY ${working_dir}
    OUTPUT_VARIABLE GIT_COMMIT
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "[git] Latest git commit(${prefix}_GIT_COMMIT): ${GIT_COMMIT}")
  add_compile_definitions(${prefix}_GIT_COMMIT="${GIT_COMMIT}")
endfunction()

# --------------------------------------------------------------------------------------------------
# Get last commit time.
function(x_git_get_latest_commit_time working_dir prefix)
  execute_process(
    COMMAND git log -1 --format=%ad --date=format:%Y.%m.%d-%H:%M:%S
    WORKING_DIRECTORY ${working_dir}
    OUTPUT_VARIABLE GIT_COMMIT_TIME
    OUTPUT_STRIP_TRAILING_WHITESPACE)
  message(STATUS "[git] Latest git commit time(${prefix}_GIT_COMMIT_TIME): ${GIT_COMMIT_TIME}")
  add_compile_definitions(${prefix}_GIT_COMMIT_TIME="${GIT_COMMIT_TIME}")
endfunction()

# --------------------------------------------------------------------------------------------------
# Add executable. It can be used by Qt5 and Qt6.
function(x_add_executable target)
  # ARGN: all unnamed arguments
  if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(${target} MANUAL_FINALIZATION ${ARGN})
  else()
    if(ANDROID)
      add_library(${target} SHARED ${ARGN})
    else()
      add_executable(${target} ${ARGN})
    endif()
  endif()

  option(X_USING_VS_CODE "Using Visual Studio Code" OFF)
  if(X_USING_VS_CODE)
    set_target_properties(xTools PROPERTIES MACOSX_BUNDLE TRUE)
  else()
    set_target_properties(xTools PROPERTIES MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE)
  endif()

  if(NOT QT_VERSION VERSION_LESS "6.8.0")
    set(macos_version ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR})
    set_target_properties(xTools PROPERTIES MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION})
    set_target_properties(xTools PROPERTIES MACOSX_BUNDLE_SHORT_VERSION_STRING ${macos_version})
  endif()
endfunction()

# --------------------------------------------------------------------------------------------------
# Generate zip file
function(x_generate_zip target version)
  if(WIN32)
    string(TOLOWER ${target} lower_target)
    string(TOLOWER ${CMAKE_HOST_SYSTEM_NAME} lower_system_name)
    string(TOLOWER ${CMAKE_SYSTEM_PROCESSOR} lower_system_processor)

    execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory ${target}-zip
                    WORKING_DIRECTORY ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../)

    add_custom_target(
      ${target}_zip
      COMMAND ${CMAKE_COMMAND} -E make_directory ${target}-zip
      COMMAND ${CMAKE_COMMAND} -E tar "cfv" ${target}-zip/${X_ASSET_NAME}.zip --format=zip ${target}
      WORKING_DIRECTORY "${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/../"
      SOURCES cmake/x.cmake)
  endif()
endfunction()

# --------------------------------------------------------------------------------------------------
# Just for test
function(x_output_env flag)
  message(STATUS "[${flag}]CMAKE_SOURCE_DIR: ${CMAKE_SOURCE_DIR}")
  message(STATUS "[${flag}]CMAKE_SOURCE_DIR: ${CMAKE_CURRENT_SOURCE_DIR}")
  message(STATUS "[${flag}]CMAKE_CURRENT_LIST_DIR: ${CMAKE_CURRENT_LIST_DIR}")
  message(STATUS "[${flag}]CMAKE_CURRENT_FUNCTION_LIST_DIR: ${CMAKE_CURRENT_FUNCTION_LIST_DIR}")
endfunction()

# --------------------------------------------------------------------------------------------------
# 3rd party libraries(Using cmake for project management)
function(x_setup_3rd_library zip_url package_name)
  # Download the package if it does not exist
  if(NOT EXISTS ${CMAKE_SOURCE_DIR}/cmake/${package_name}.zip)
    message(STATUS "Downloading ${package_name} from ${zip_url}")
    file(
      DOWNLOAD ${zip_url} ${CMAKE_SOURCE_DIR}/3rd/${package_name}.zip
      SHOW_PROGRESS
      STATUS status)
    if(NOT status EQUAL 0)
      message(FATAL_ERROR "Failed to download ${package_name} from ${package_url}")
    endif()
  endif()

  # Extract the package
  if(NOT EXISTS ${CMAKE_SOURCE_DIR}/3rd/${package_name})
    message(STATUS "Extracting ${package_name}")
    execute_process(
      COMMAND ${CMAKE_COMMAND} -E tar "xzf" ${CMAKE_SOURCE_DIR}/3rd/${package_name}.zip
      WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}/3rd
      RESULT_VARIABLE result)
    if(result)
      message(FATAL_ERROR "Failed to extract ${package_name}")
    endif()
  endif()
endfunction()

# --------------------------------------------------------------------------------------------------
# Install 3rd party libraries
function(x_install_3rd_library target_name dir_name)
  add_custom_target(
    ${target_name}-deploy
    COMMAND ${CMAKE_COMMAND} --install . --prefix ${X_LIBS_DIR}/${dir_name}
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/3rd/${dir_name}
    COMMENT "Deploy 3rd libraries")
endfunction()
