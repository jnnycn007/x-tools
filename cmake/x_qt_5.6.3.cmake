# https://github.com/x-tools-author/x-tools/releases/download/v7.6.1/Qt5.6.3.7z
# https://master.dl.sourceforge.net/project/fsu0413-qtbuilds/Qt5.6/Windows-x86/Qt5.6.3-Windows-x86-MinGW4.9.4-20200104.7z?viasf=1&fid=bd25050269beb2b4&e=1787807811&st=VDptmIzHuJTdnaPPFkP2KA
# https://download.qt.io/online/qtsdkrepository/windows_x86/desktop/tools_mingw/qt.tools.win32_mingw492/4.9.2-1i686-4.9.2-release-posix-dwarf-rt_v3-rev1.7z
set(qt_5_6_3 "Qt5.6.3")
set(qt_5_6_3_url "https://github.com/x-tools-author/x-tools/releases/download/v7.6.1")
set(qt_5_6_3_url "${qt_5_6_3_url}/${qt_5_6_3}.7z")
set(dst_path "${CMAKE_SOURCE_DIR}/tools/${qt_5_6_3}.7z")
if(NOT EXISTS ${dst_path} AND NOT EXISTS "${CMAKE_SOURCE_DIR}/tools/${qt_5_6_3}")
  # Download Qt 5.6.3
  message(STATUS "[xTools.WinXP] Downloading Qt 5.6.3...")
  file(
    DOWNLOAD ${qt_5_6_3_url} ${dst_path}
    SHOW_PROGRESS
    STATUS download_status)
  list(GET download_status 0 download_result)
  if(NOT download_result EQUAL 0)
    file(REMOVE ${dst_path})
    message(FATAL_ERROR "[xTools.WinXP] Failed to download Qt 5.6.3 from ${qt_5_6_3_url}")
  endif()

  # Extract Qt 5.6.3
  message(STATUS "[xTools.WinXP] Extracting Qt 5.6.3...")
  execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${dst_path}
                  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/")
  execute_process(COMMAND ${CMAKE_SOURCE_DIR}/tools/${qt_5_6_3}/5.6.3/mingw49_32/QQtPatcher.exe)
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/tools/${qt_5_6_3}")
  if(EXISTS ${dst_path})
    execute_process(COMMAND ${CMAKE_COMMAND} -E tar xf ${dst_path}
                    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/")
  endif()
endif()

if(NOT EXISTS "${CMAKE_SOURCE_DIR}/tools/${qt_5_6_3}")
  message(FATAL_ERROR "[xTools.WinXP] Qt 5.6.3 setup failed.")
endif()

# Apply Qt 5.6.3 patcher(Maybe you should run it manually).
message(STATUS "[xTools.WinXP] Applying Qt 5.6.3 patcher...")
execute_process(COMMAND ${CMAKE_SOURCE_DIR}/tools/${qt_5_6_3}/5.6.3/mingw49_32/QQtPatcher.exe
                WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/tools/${qt_5_6_3}/5.6.3/mingw49_32/")
