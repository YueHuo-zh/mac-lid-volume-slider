cmake_minimum_required(VERSION 3.31)

project(MacLidVolumeSlider LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BUILD_EXAMPLE OFF)

add_executable(MacLidVolumeSlider
    src/main.cpp
    src/audio_volume.cpp
    src/volume_mapping.cpp
)

target_compile_options(MacLidVolumeSlider PRIVATE -Wall -Wextra -Wpedantic)

find_library(COREAUDIO_FRAMEWORK CoreAudio REQUIRED)
target_link_libraries(MacLidVolumeSlider PRIVATE ${COREAUDIO_FRAMEWORK})

add_subdirectory(third_party/mac-angle EXCLUDE_FROM_ALL)
target_link_libraries(MacLidVolumeSlider PRIVATE lid_angle)

include(CTest)
if(BUILD_TESTING)
    add_executable(VolumeMappingTests tests/volume_mapping_test.cpp src/volume_mapping.cpp)
    target_include_directories(VolumeMappingTests PRIVATE src)
    target_compile_options(VolumeMappingTests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME volume_mapping COMMAND VolumeMappingTests)

    # 使用真实入口和模拟硬件，覆盖重复读数、设备切换及预览模式。
    add_executable(ControllerTests src/main.cpp src/volume_mapping.cpp tests/controller_backend.cpp)
    target_include_directories(ControllerTests PRIVATE src third_party/mac-angle)
    target_compile_options(ControllerTests PRIVATE -Wall -Wextra -Wpedantic)
    add_test(NAME controller COMMAND ControllerTests)
    add_test(NAME controller_preview COMMAND ControllerTests --dry-run)
    set_tests_properties(controller_preview PROPERTIES ENVIRONMENT "SLIDER_TEST_PREVIEW=1")
    set_tests_properties(controller controller_preview PROPERTIES TIMEOUT 5)
endif()
