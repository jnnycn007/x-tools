﻿# https://codeload.github.com/bricke/Qt-AES/zip/refs/heads/master
set(packet_version master)
set(packet_name Qt-AES-${packet_version})
set(packet_url "https://codeload.github.com/bricke/Qt-AES/zip/refs/heads/${packet_version}")

if(NOT EXISTS ${CMAKE_SOURCE_DIR}/3rd/${packet_name}.zip)
  message(STATUS "Downloading ${packet_name} from ${packet_url}")
  file(
    DOWNLOAD ${packet_url} ${CMAKE_SOURCE_DIR}/3rd/${packet_name}.zip
    STATUS download_status
    SHOW_PROGRESS)
  # Get the status code from download_status(such as "0;noerror")
  message(STATUS "Download status: ${download_status}")
  list(GET download_status 0 status_code)
  if(NOT status_code EQUAL 0)
    message(FATAL_ERROR "Failed to download ${packet_name}: ${download_status}")
  endif()
endif()

if(NOT EXISTS ${X_3RD_DIR}/${packet_name})
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xzf ${packet_name}.zip
                  WORKING_DIRECTORY ${X_3RD_DIR})
endif()

x_auto_import_package_dir(${packet_name} QtAES)
list(APPEND X_LIBS QtAES::QtAES)
