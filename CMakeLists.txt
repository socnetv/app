# SPDX-License-Identifier: BSD-3-Clause

# Set the minimum cmake version here:
cmake_minimum_required(VERSION 3.16)

# Set our project name and version here:
project(SocNetV 
    VERSION 3.2
    DESCRIPTION "SocNetV: Open-source social network analysis application based on Qt."
    HOMEPAGE_URL "https://socnetv.org"
    LANGUAGES CXX)

# Other way to set project version
# set(PROJECT_VERSION_MAJOR 3)
# set(PROJECT_VERSION_MINOR 2)
# set(PROJECT_VERSION_PATCH 0)

# Set the CMAKE_CXX_STANDARD 
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Some options here:
option(DEV_MODE "Build in developer mode" OFF)

# Echo some debug messages for sanity check:
message(STATUS "####")
message(STATUS "#### Variable checks...")
message(STATUS "####")
message(STATUS "PROJECT_NAME: " ${PROJECT_NAME})
message(STATUS "PROJECT_VERSION: "  ${PROJECT_VERSION})
message(STATUS "CMAKE_SOURCE_DIR: "  ${CMAKE_SOURCE_DIR})
message(STATUS "CMAKE_CURRENT_LIST_DIR: "  ${CMAKE_CURRENT_LIST_DIR})
message(STATUS "CMAKE_CURRENT_BINARY_DIR: "  ${CMAKE_CURRENT_BINARY_DIR})
# set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})
message(STATUS "CMAKE_RUNTIME_OUTPUT_DIRECTORY: "  ${CMAKE_RUNTIME_OUTPUT_DIRECTORY})
message(STATUS "CMAKE_MODULE_PATH: "  ${CMAKE_MODULE_PATH})

# Prepare compile time flags
set(gcc_like_cxx "$<COMPILE_LANG_AND_ID:CXX,ARMClang,AppleClang,Clang,GNU,LCC>")
set(msvc_cxx "$<COMPILE_LANG_AND_ID:CXX,MSVC>")

message(STATUS "For GCC I will use these flags: " ${gcc_like_cxx})
message(STATUS "For MSVC I will use these flags: " ${msvc_cxx})

message(CHECK_START "Checking INSTALL_DIR")

if(NOT DEFINED INSTALL_DIR)
    message(CHECK_FAIL "None")
    set(INSTALL_DIR "${CMAKE_CURRENT_SOURCE_DIR}/build")
    message(CHECK_START "Setting INSTALL_DIR")
    message(CHECK_PASS ${INSTALL_DIR})
else()
    message(CHECK_SUCCESS ${INSTALL_DIR})
endif()



message(STATUS "####")
message(STATUS "#### Finding Qt6 and other packages ...")
message(STATUS "####")

find_package(Qt6 REQUIRED COMPONENTS Core OpenGLWidgets Gui Core5Compat Widgets PrintSupport Network Charts Svg Xml)



message(STATUS "####")
message(STATUS "#### Running standard Qt project setup ...")
message(STATUS "####")

qt_standard_project_setup()



message(STATUS "####")
message(STATUS "#### Adding sources and executable ...")
message(STATUS "####")

set(SOURCE_FILES  
    src/forms/dialogfilteredgesbyweight.ui 
    src/forms/dialogsettings.ui 
    src/forms/dialogsysteminfo.ui 
    src/forms/dialogwebcrawler.ui 
    src/forms/dialogdatasetselect.ui 
    src/forms/dialograndsmallworld.ui 
    src/forms/dialograndscalefree.ui 
    src/forms/dialogranderdosrenyi.ui 
    src/forms/dialograndregular.ui 
    src/forms/dialograndlattice.ui 
    src/forms/dialogsimilaritypearson.ui 
    src/forms/dialogsimilaritymatches.ui 
    src/forms/dialogdissimilarities.ui 
    src/forms/dialogclusteringhierarchical.ui   
    src/forms/dialognodeedit.ui 
    src/forms/dialognodefind.ui 
    src/forms/dialogedgedichotomization.ui 
    src/forms/dialogexportpdf.ui 
    src/forms/dialogexportimage.ui
)

# # Headers
# list(APPEND SOURCE_FILES
#     src/mainwindow.h 
#     src/texteditor.h 
#     src/graph.h 
#     src/graphvertex.h 
#     src/matrix.h 
#     src/parser.h 
#     src/webcrawler.h 
#     src/chart.h 
#     src/graphicswidget.h 
#     src/graphicsedge.h 
#     src/graphicsedgeweight.h 
#     src/graphicsedgelabel.h 
#     src/graphicsguide.h 
#     src/graphicsnode.h 
#     src/graphicsnodelabel.h 
#     src/graphicsnodenumber.h 
#     src/forms/dialogfilteredgesbyweight.h 
#     src/forms/dialogedgedichotomization.h 
#     src/forms/dialogwebcrawler.h 
#     src/forms/dialogdatasetselect.h 
#     src/forms/dialogpreviewfile.h 
#     src/forms/dialognodeedit.h 
#     src/forms/dialogranderdosrenyi.h 
#     src/forms/dialograndsmallworld.h 
#     src/forms/dialograndscalefree.h 
#     src/forms/dialograndregular.h 
#     src/forms/dialogsettings.h 
#     src/forms/dialogsimilaritypearson.h 
#     src/forms/dialogsimilaritymatches.h 
#     src/forms/dialogdissimilarities.h 
#     src/forms/dialogclusteringhierarchical.h  
#     src/forms/dialograndlattice.h 
#     src/forms/dialognodefind.h 
#     src/forms/dialogexportpdf.h 
#     src/forms/dialogexportimage.h 
#     src/forms/dialogsysteminfo.h 
#     src/global.h
# )
    
    
list(APPEND SOURCE_FILES
    src/main.cpp 
    src/mainwindow.cpp 
    src/texteditor.cpp 
    src/graph.cpp 
    src/graphvertex.cpp 
    src/matrix.cpp 
    src/parser.cpp 
    src/webcrawler.cpp 
    src/chart.cpp 
    src/graphicswidget.cpp 
    src/graphicsedge.cpp 
    src/graphicsedgeweight.cpp 
    src/graphicsedgelabel.cpp 
    src/graphicsguide.cpp 
    src/graphicsnode.cpp 
    src/graphicsnodelabel.cpp 
    src/graphicsnodenumber.cpp 
    src/forms/dialogfilteredgesbyweight.cpp 
    src/forms/dialogedgedichotomization.cpp 
    src/forms/dialogwebcrawler.cpp 
    src/forms/dialogdatasetselect.cpp 
    src/forms/dialogpreviewfile.cpp 
    src/forms/dialognodeedit.cpp 
    src/forms/dialogranderdosrenyi.cpp 
    src/forms/dialograndsmallworld.cpp 
    src/forms/dialograndregular.cpp 
    src/forms/dialograndscalefree.cpp 
    src/forms/dialogsettings.cpp 
    src/forms/dialogsimilaritypearson.cpp 
    src/forms/dialogsimilaritymatches.cpp  
    src/forms/dialogdissimilarities.cpp 
    src/forms/dialogclusteringhierarchical.cpp  
    src/forms/dialograndlattice.cpp 
    src/forms/dialognodefind.cpp 
    src/forms/dialogexportpdf.cpp 
    src/forms/dialogexportimage.cpp 
    src/forms/dialogsysteminfo.cpp
 )

# List source files
message(STATUS "Source files to be compiled: ${SOURCE_FILES}")

# Add Qt resource files
qt6_add_resources(RESOURCES 
#  ./src/application.qrc # obsolete ? 
    ./src/src.qrc
)
 
add_executable(${PROJECT_NAME} ${SOURCE_FILES} ${RESOURCES})

# qt_add_executable(${PROJECT_NAME}     ${SOURCE_FILES} ${RESOURCES} )


# Add include directories for headers (equivalent to INCLUDEPATH)
target_include_directories(${PROJECT_NAME}  PUBLIC
                           "${CMAKE_SOURCE_DIR}/src"
                           )

message(STATUS "Include paths: ${CMAKE_SOURCE_DIR}/src")



message(STATUS "####")
message(STATUS "#### Setting target properties and compile time options ...")
message(STATUS "####")


set_target_properties(${PROJECT_NAME} PROPERTIES
    WIN32_EXECUTABLE TRUE
    MACOSX_BUNDLE TRUE
    CXX_STANDARD 17
    CXX_STANDARD_REQUIRED ON
    AUTOMOC ON
    AUTOUIC ON
    AUTORCC ON
)



if (DEV_MODE) 
    message(STATUS "DEV_MODE build configuration detected")

    else()
        message(STATUS "NORMAL build configuration detected")
        # target_compile_options(${PROJECT_NAME} INTERFACE
        # "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
        # "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
        # )
endif()


if (CMAKE_BUILD_TYPE MATCHES Debug)
    message(STATUS "Debug build configuration detected. Adding -DQT_DEBUG and compile flags...")
    add_definitions(-DQT_DEBUG)
    target_compile_options(${PROJECT_NAME} INTERFACE
        "$<${gcc_like_cxx}:$<BUILD_INTERFACE:-Wall;-Wextra;-Wshadow;-Wformat=2;-Wunused>>"
        "$<${msvc_cxx}:$<BUILD_INTERFACE:-W3>>"
    )
else()
    message(STATUS "Non-debug build configuration detected")
endif()

#
#  SET OS Specific things here
#
if(WINDOWS)
    message(STATUS "Setting up for building on WINDOWS...")
endif()

if(APPLE AND NOT IOS)
    message(STATUS "Setting up for building on APPLE...")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/macos/Info.plist.in"
    )
elseif(IOS)
    message(STATUS "Setting up for building on IOS...")
    set_target_properties(${PROJECT_NAME} PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/ios/Info.plist.in"
    )
endif()


if(LINUX)
    message(STATUS "Setting up for building on LINUX...")
    # target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra -O2)
endif()


get_target_property(COMPILE_OPTIONS ${PROJECT_NAME} COMPILE_OPTIONS)
message(STATUS "Compile options for ${PROJECT_NAME}: ${COMPILE_OPTIONS}")


target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::OpenGLWidgets  
    Qt6::Core5Compat 
    Qt6::PrintSupport 
    Qt6::Network 
    Qt6::Charts 
    Qt6::Svg 
    Qt6::Xml
)


# Finalize:
# qt_finalize_executable(${PROJECT_NAME})

install(TARGETS ${PROJECT_NAME}
    # RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    # BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    # LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
