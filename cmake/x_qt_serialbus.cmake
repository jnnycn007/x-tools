find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS SerialBus)

macro(remove_all_x_modbus_files)
  file(GLOB_RECURSE MODBUS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/x/modbus/*.*")
  foreach(file ${MODBUS_SOURCE})
    list(REMOVE_ITEM X_SOURCES ${file})
    message(STATUS "[Modbus]Remove file: ${file}")
  endforeach(file ${MODBUS_SOURCE})
endmacro()

macro(remove_all_x_canbus_files)
  file(GLOB_RECURSE CANBUS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/x/canbus/*.*")
  foreach(file ${CANBUS_SOURCE})
    list(REMOVE_ITEM X_SOURCES ${file})
    message(STATUS "[Canbus]Remove file: ${file}")
  endforeach(file ${CANBUS_SOURCE})
endmacro()

if(Qt${QT_VERSION_MAJOR}SerialBus_FOUND AND NOT QT_VERSION VERSION_LESS "5.12.0")
  add_compile_definitions(X_ENABLE_SERIALBUS)
  list(APPEND X_LIBS Qt${QT_VERSION_MAJOR}::SerialBus)

  option(X_ENABLE_X_MODBUS "Enable xModbus module" ON)
  if(X_ENABLE_X_MODBUS)
    add_compile_definitions(X_ENABLE_X_MODBUS=1)
  else()
    remove_all_x_modbus_files()
    add_compile_definitions(X_ENABLE_X_MODBUS=0)
  endif()

  option(X_ENABLE_X_CANBUS "Enable xCanbus module" ON)
  if(X_ENABLE_X_CANBUS)
    add_compile_definitions(X_ENABLE_X_CANBUS=1)
  else()
    remove_all_x_canbus_files()
    add_compile_definitions(X_ENABLE_X_CANBUS=0)
  endif()
else()
  message(STATUS "SerialBus module is disable, SerialBus files will be removed.")
  # Remove modbus files
  file(GLOB_RECURSE MODBUS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/tools/modbus/*.*")
  foreach(file ${MODBUS_SOURCE})
    list(REMOVE_ITEM X_SOURCES ${file})
    message(STATUS "[Modbus]Remove file: ${file}")
  endforeach(file ${MODBUS_SOURCE})

  add_compile_definitions(X_ENABLE_X_MODBUS=0)
  remove_all_x_modbus_files()

  # Remove canbus files
  file(GLOB_RECURSE CANBUS_SOURCE "${CMAKE_CURRENT_SOURCE_DIR}/src/tools/canbus/*.*")
  foreach(file ${CANBUS_SOURCE})
    list(REMOVE_ITEM X_SOURCES ${file})
    message(STATUS "[Canbus]Remove file: ${file}")
  endforeach(file ${CANBUS_SOURCE})

  add_compile_definitions(X_ENABLE_X_CANBUS=0)
  remove_all_x_canbus_files()
endif()

# https://github.com/x-tools-author/x-tools-dependencies/releases/download/dependencies/PCAN-Basic-V5.1.0.1194.zip
set(PCAN_BASE_URL_BASE "https://github.com/x-tools-author/x-tools-dependencies/releases")
set(PCAN_BASE_URL_BASE "${PCAN_BASE_URL_BASE}/download/dependencies")
set(PCAN_BASE_VERSION "5.1.0.1194")
set(PCAN_BASE_NAME "PCAN-Basic-V${PCAN_BASE_VERSION}")
set(PCAN_BASE_URL "${PCAN_BASE_URL_BASE}/${PCAN_BASE_NAME}.zip")

function(x_auto_deploy_pcan_base target)
  if(NOT WIN32)
    return()
  endif()

  if(CMAKE_SIZEOF_VOID_P EQUAL 8) # x64
    set(PCAN_BASE_DLL "${X_3RD_DIR}/${PCAN_BASE_NAME}/x64/PCANBasic.dll")
  else() # x86
    set(PCAN_BASE_DLL "${X_3RD_DIR}/${PCAN_BASE_NAME}/x86/PCANBasic.dll")
  endif()

  add_custom_command(
    TARGET ${target}
    POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different "${PCAN_BASE_DLL}" $<TARGET_FILE_DIR:${target}>)
endfunction()

# --------------------------------------------------------------------------------------------------
# PCAN-Base, just for Windows platform yet...
if(NOT WIN32)
  return()
endif()

# --------------------------------------------------------------------------------------------------
# Download the PCAN-Base repository if it does not exist
if(NOT EXISTS "${X_3RD_DIR}/${PCAN_BASE_NAME}.zip")
  message(STATUS "Downloading ${PCAN_BASE_NAME} from ${PCAN_BASE_URL}")
  file(
    DOWNLOAD "${PCAN_BASE_URL}" "${X_3RD_DIR}/${PCAN_BASE_NAME}.zip"
    SHOW_PROGRESS
    STATUS DOWNLOAD_STATUS)
  list(GET DOWNLOAD_STATUS 0 DOWNLOAD_RESULT)
  if(NOT DOWNLOAD_RESULT EQUAL 0)
    message(WARNING "Failed to download ${PCAN_BASE_NAME} from ${PCAN_BASE_URL}")
    message(WARNING "PeakCAN plugin will be disabled.")
    # Remove pcan-base files
    file(REMOVE "${X_3RD_DIR}/${PCAN_BASE_NAME}.zip")
    return()
  endif()
endif()

# --------------------------------------------------------------------------------------------------
# Unzip the PCAN-Base repository if it does not exist
if(NOT EXISTS "${X_3RD_DIR}/${PCAN_BASE_NAME}/ReadMe.txt")
  message(STATUS "Unzipping ${PCAN_BASE_NAME}...")
  execute_process(COMMAND ${CMAKE_COMMAND} -E make_directory "${X_3RD_DIR}/${PCAN_BASE_NAME}")
  execute_process(
    COMMAND ${CMAKE_COMMAND} -E tar xzf "${X_3RD_DIR}/${PCAN_BASE_NAME}.zip"
    WORKING_DIRECTORY "${X_3RD_DIR}/${PCAN_BASE_NAME}"
    RESULT_VARIABLE UNZIP_RESULT)
  if(NOT UNZIP_RESULT EQUAL 0)
    message(WARNING "Failed to unzip ${PCAN_BASE_NAME}")
    message(WARNING "PeakCAN plugin will be disabled.")
    # Remove pcan-base files
    file(REMOVE "${X_3RD_DIR}/${PCAN_BASE_NAME}.zip")
    return()
  endif()
endif()
