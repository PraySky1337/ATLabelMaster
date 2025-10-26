cmake_minimum_required(VERSION 3.16)

project(LabelMaster LANGUAGES CXX)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(SRC_PATH ${CMAKE_CURRENT_SOURCE_DIR}/labelmaster/src)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOUIC_SEARCH_PATHS
    ${CMAKE_CURRENT_SOURCE_DIR}/labelmaster/src/ui
)

set(EXECUTABLE_OUTPUT_PATH ${CMAKE_SOURCE_DIR}/bin)

file(GLOB_RECURSE SRC_FILES CONFIGURE_DEPENDS
    ${SRC_PATH}/*.cpp
    ${SRC_PATH}/*.hpp
    ${SRC_PATH}/*.ui
)

find_package(OpenCV REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS Widgets Core Gui)

add_executable(${PROJECT_NAME} ${SRC_FILES})

target_include_directories(${PROJECT_NAME} PRIVATE 
    ${SRC_PATH}
    ${OpenCV_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE 
    Qt6::Widgets 
    Qt6::Core
    Qt6::Gui
    ${OpenCV_LIBS}
)

install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION /usr/bin
)
install(DIRECTORY resources/ DESTINATION /usr/share/${PROJECT_NAME}/resources)