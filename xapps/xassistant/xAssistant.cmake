﻿message(STATUS "[xAssistant]xAssistant information...")
message(STATUS "[xAssistant]Libs: ${X_LIBS}")

# Remove all main.cpp xTools.rc  files from X_SOURCES
set(X_APPS_SOURCES ${X_SOURCES})
foreach(source ${X_APPS_SOURCES})
  if(${source} MATCHES "main.cpp")
    list(REMOVE_ITEM X_APPS_SOURCES ${source})
  endif()
  if(${source} MATCHES "xTools.rc")
    list(REMOVE_ITEM X_APPS_SOURCES ${source})
  endif()
endforeach()

file(GLOB_RECURSE X_ASSISTANT_SOURCES ${CMAKE_CURRENT_LIST_DIR}/src/*.*)
list(APPEND X_ASSISTANT_SOURCES ${X_APPS_SOURCES})
list(APPEND X_ASSISTANT_SOURCES ${CMAKE_CURRENT_LIST_DIR}/xAssistant.rc)
list(APPEND X_ASSISTANT_SOURCES ${CMAKE_CURRENT_LIST_DIR}/xAssistant.qrc)
include_directories(${CMAKE_CURRENT_LIST_DIR}/src)

set(bin ${CMAKE_CURRENT_SOURCE_DIR}/bin/${CMAKE_SYSTEM_NAME}/${CMAKE_BUILD_TYPE}/xAssistant)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${bin})
qt_add_executable(xAssistant ${X_ASSISTANT_SOURCES})
x_output_env(xAssistant)
x_deploy_qt(xAssistant)
target_link_libraries(xAssistant PRIVATE ${X_LIBS})
if(X_USING_VS_CODE)
  set_target_properties(xAssistant PROPERTIES MACOSX_BUNDLE TRUE)
else()
  set_target_properties(xAssistant PROPERTIES MACOSX_BUNDLE TRUE WIN32_EXECUTABLE TRUE)
endif()

if(WIN32 AND MSVC)
  target_link_libraries(xAssistant PRIVATE Dwmapi)
elseif(LINUX)
  target_link_libraries(xAssistant PRIVATE dl)
endif()

# --------------------------------------------------------------------------------------------------
# Make installer for Windows
set(X_ASSISTANT_VERSION "1.3.0")
add_compile_definitions(X_ASSISTANT_VERSION="${X_ASSISTANT_VERSION}")
if(WIN32)
  include(${CMAKE_SOURCE_DIR}/cmake/msix/msix.cmake)
  include(${CMAKE_SOURCE_DIR}/cmake/qifw/qifw.cmake)
  set(icon ${CMAKE_CURRENT_SOURCE_DIR}/xAssistant.ico)
  x_generate_zip(xAssistant ${X_ASSISTANT_VERSION})
  x_generate_msix(xAssistant "xAssistant" "xAssistant" ${X_ASSISTANT_VERSION} FALSE)
  x_generate_installer(xAssistant ${X_ASSISTANT_VERSION} ${icon})
endif()
