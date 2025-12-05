cmake_minimum_required(VERSION 3.20)

# PROJECT BASIC INFO
project(MicroSlide 
    VERSION 1.0.0
    DESCRIPTION "MicroSlide Scanner Software"
    LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# QT AUTOMIC SETTING
set(CMAKE_AUTOMOC ON)	
set(CMAKE_AUTOUIC ON) 	
set(CMAKE_AUTORCC ON) 	
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_SOURCE_DIR}/assets/ui
)

# SET OUTPUT DIR
if(CMAKE_SIZEOF_VOID_P EQUAL 8)
    set(PLATFORM "x64")
else()
    set(PLATFORM "x86")
endif()

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM}/$<CONFIG>)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM}/$<CONFIG>)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_SOURCE_DIR}/bin/${PLATFORM}/$<CONFIG>)

# FIND DEPENDENCIES
# FIND QT LIB
set(CMAKE_PREFIX_PATH "C:/My/Tools/Qt/6.7.3/msvc2022_64")
find_package(Qt6 REQUIRED COMPONENTS 
    Core 
    Gui 
    Widgets 
    Multimedia 
    MultimediaWidgets
    Sql
)

set(HEADERS
    src/basethread.h
    src/bgcamera.h
    src/CamImgPool.h
    src/db_raw.h
    src/deployment.h
    src/filetest.h
    src/hpec_lib.h
    src/IniParser.h
    src/magdebug.h
    src/microslidewindow.h
    src/scanner_processor.h
    src/sysconfigform.h
    src/tmagnifydetect.h
)
set(SOURCES
    src/basethread.cpp
    src/bgcamera.cpp
    src/CamImgPool.cpp
    src/deployment.cpp
    src/filetest.cpp
    src/hpec_lib.cpp
    src/IniParser.cpp
    src/magdebug.cpp
	src/main.cpp
    src/microslidewindow.cpp
    src/scanner_processor.cpp
    src/sysconfigform.cpp
    src/tmagnifydetect.cpp
)


# GENERATE AN EXECUTABLE FILE
add_executable(${PROJECT_NAME}
    WIN32 
    ${SOURCES}
    ${HEADERS}
    assets/icons/MicroSlide.rc
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    WIN32_LEAN_AND_MEAN     # 减少 Windows.h 包含的内容
    _WIN32_WINNT=0x0A00     # Windows 10
    _WINSOCKAPI_            # 防止包含 winsock.h
)

# 添加链接器标志以支持 /GL 编译的库
if(MSVC)
    target_link_options(${PROJECT_NAME} PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Debug>:/INCREMENTAL:NO>
    )
endif()

# INCLUDE
target_include_directories(${PROJECT_NAME} PRIVATE
    src
    external/json/include
    external/opencv-4.10.0/build/include
    external/newland/include
    external/onnxruntime-win-x64-1.14.1/include
    external/DBInterfaceDLL/include
    external/hpec_lib/include
    external/bgimaging/include
)


# TARGET LINK LIB
target_link_libraries(${PROJECT_NAME} PRIVATE
	Qt6::Core
	Qt6::Gui
	Qt6::Widgets
    Qt6::Multimedia
    Qt6::MultimediaWidgets
    Qt6::Sql
)

target_link_directories(${PROJECT_NAME} PRIVATE
    src/lib
    external/opencv-4.10.0/build/x64/vc16/lib
    external/newland/lib/x64
    external/onnxruntime-win-x64-1.14.1/lib
    external/DBInterfaceDLL/lib
    external/hpec_lib/lib
    external/bgimaging/lib
)

# LINK OPENCV LIBS
target_link_libraries(${PROJECT_NAME} PRIVATE
    bgimaging
    DBInterfaceDLL
    N-ScanHub
    onnxruntime
    opencv_world4100$<$<CONFIG:Debug>:d>
    hl_lib$<$<CONFIG:Debug>:d>
    DBInterfaceDLL$<$<CONFIG:Debug>:d>
    # $<$<CONFIG:Debug>:opencv_world4100d>
    # $<$<CONFIG:Release>:opencv_world4100>
    # $<$<CONFIG:Debug>:hl_libd>
    # $<$<CONFIG:Release>:hl_lib>
    # $<$<CONFIG:Debug>:DBInterfaceDLLd>
    # $<$<CONFIG:Release>:DBInterfaceDLL>
)

# 复制运行时依赖的 DLL 到输出目录
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    # OpenCV DLLs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/opencv-4.10.0/build/x64/vc16/bin/opencv_world4100$<$<CONFIG:Debug>:d>.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/opencv-4.10.0/build/x64/vc16/bin/opencv_videoio_ffmpeg4100_64.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    
    # ONNX Runtime DLLs
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/onnxruntime-win-x64-1.14.1/lib/onnxruntime.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/onnxruntime-win-x64-1.14.1/lib/onnxruntime_providers_shared.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    
    # Newland DLL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/newland/lib/x64/N-ScanHub.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    
    # DBInterfaceDLL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/DBInterfaceDLL/lib/DBInterfaceDLL$<$<CONFIG:Debug>:d>.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    
    # bgimaging DLL
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_SOURCE_DIR}/external/bgimaging/lib/bgimaging.dll"
        "$<TARGET_FILE_DIR:${PROJECT_NAME}>"
    
    COMMENT "Copying DLL files to output directory"
)




