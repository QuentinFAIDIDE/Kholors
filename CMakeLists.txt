# Example GUI App CMakeLists.txt

# To get started on a new GUI app, copy this entire folder (containing this file and C++ sources) to
# a convenient location, and then start making modifications. For other examples of CMakeLists for
# GUI apps, check `extras/Projucer` and `examples/DemoRunner` in the JUCE repo.

# The first line of any CMake project should be a call to `cmake_minimum_required`, which checks
# that the installed CMake will be able to understand the following CMakeLists, and ensures that
# CMake's behaviour is compatible with the named version. This is a standard CMake command, so more
# information can be found in the CMake docs.
cmake_minimum_required(VERSION 3.15)
set(CMAKE_CXX_STANDARD 17)

# For clang tidy and vscode to file compilation args
set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 
set(CMAKE_CXX_CLANG_TIDY "clang-tidy;-p=${CMAKE_BINARY_DIR},-config=.clang-format")

# try to use ccache if possible
find_program(CCACHE_FOUND ccache)
if(CCACHE_FOUND)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_COMPILE ccache)
    set_property(GLOBAL PROPERTY RULE_LAUNCH_LINK ccache) # Less useful to do it for linking, see edit2
endif(CCACHE_FOUND)

# to use gdb
set(CMAKE_BUILD_TYPE Debug)

# The top-level CMakeLists.txt file for a project must contain a literal, direct call to the
# `project()` command. `project()` sets up some helpful variables that describe source/binary
# directories, and the current project version. This is a standard CMake command.

project(Kholors VERSION 0.0.1)

# If you've installed JUCE somehow (via a package manager, or directly using the CMake install
# target), you'll need to tell this project that it depends on the installed copy of JUCE. If you've
# included JUCE directly in your source tree (perhaps as a submodule), you'll need to tell CMake to
# include that subdirectory as part of the build.

# find_package(JUCE CONFIG REQUIRED)        # If you've installed JUCE to your system
# or
add_subdirectory(JUCE)                    # If you've put JUCE in a subdirectory called JUCE


# ensure the required packages are installed
find_package(nlohmann_json 3.11.2 REQUIRED)
find_package(yaml-cpp)
find_package(OpenSSL)

# Include tests and Ctest utility
include(CTest)
add_test(NAME TestConfig COMMAND TestConfig)
add_test(NAME TestAudioLib COMMAND TestAudioLib)
add_test(NAME TestSamplePlayer COMMAND TestSamplePlayer)
add_test(NAME TestUnitConverter COMMAND TestUnitConverter)
add_test(NAME TestTextureManager COMMAND TestTextureManager)
add_test(NAME TestGitWrapper COMMAND TestGitWrapper)

# If your app depends the VST2 SDK, perhaps to host VST2 plugins, CMake needs to be told where
# to find the SDK on your system. This setup should be done before calling `juce_add_gui_app`.

# juce_set_vst2_sdk_path(...)

# `juce_add_gui_app` adds an executable target with the name passed as the first argument
# (GuiAppExample here). This target is a normal CMake target, but has a lot of extra properties set
# up by default. This function accepts many optional arguments. Check the readme at
# `docs/CMake API.md` in the JUCE repo for the full list.

juce_add_gui_app(Kholors
    # VERSION ...                       # Set this if the app version is different to the project version
    # ICON_BIG ...                      # ICON_* arguments specify a path to an image file to use as an icon
    # ICON_SMALL ...
    # DOCUMENT_EXTENSIONS ...           # Specify file extensions that should be associated with this app
    # COMPANY_NAME ...                  # Specify the name of the app's author
    PRODUCT_NAME "Kholors")     # The name of the final executable, which can differ from the target name

juce_add_gui_app(TestConfig PRODUCT_NAME "TestConfig")
juce_add_gui_app(TestAudioLib PRODUCT_NAME "TestAudioLib")
juce_add_gui_app(TestSamplePlayer PRODUCT_NAME "TestSamplePlayer")
juce_add_gui_app(TestUnitConverter PRODUCT_NAME "TestUnitConverter")
juce_add_gui_app(TestTextureManager PRODUCT_NAME "TestTextureManager")
juce_add_gui_app(TestGitWrapper PRODUCT_NAME "TestGitWrapper")

# `juce_generate_juce_header` will create a JuceHeader.h for a given target, which will be generated
# into your build tree. This should be included with `#include <JuceHeader.h>`. The include path for
# this header will be automatically added to the target. The main function of the JuceHeader is to
# include all your JUCE module headers; if you're happy to include module headers directly, you
# probably don't need to call this.

# juce_generate_juce_header(GuiAppExample)

# `target_sources` adds source files to a target. We pass the target that needs the sources as the
# first argument, then a visibility parameter for the sources which should normally be PRIVATE.
# Finally, we supply a list of source files that will be built into the target. This is a standard
# CMake command.
file(GLOB_RECURSE ROOT_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*.cpp")
file(GLOB_RECURSE SUBDIR_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*/*.cpp")
file(GLOB_RECURSE SUBSUBDIR_SOURCES RELATIVE ${CMAKE_SOURCE_DIR} "src/*/*/*.cpp")

target_sources(Kholors PRIVATE ${ROOT_SOURCES} ${SUBDIR_SOURCES} ${SUBSUBDIR_SOURCES})

target_sources(TestConfig
    PRIVATE
        src/Config.cpp
        test/TestConfig.cpp)

target_sources(TestGitWrapper
    PRIVATE
        src/Arrangement/GitWrapper.cpp
        src/Config.cpp
        test/TestGitWrapper.cpp)

target_sources(TestTextureManager
    PRIVATE
        src/OpenGL/TextureManager.cpp
        src/Audio/SamplePlayer.cpp
        test/TestTextureManager.cpp
        src/Audio/AudioFilesBufferStore.cpp
        src/Audio/UnitConverter.cpp)

target_sources(TestAudioLib
    PRIVATE
        src/Library/AudioLibraryManager.cpp
        test/TestAudioLibrary.cpp)

target_sources(TestSamplePlayer
    PRIVATE
        src/Audio/SamplePlayer.cpp
        test/TestSamplePlayer.cpp
        src/Audio/UnitConverter.cpp
        src/Audio/AudioFilesBufferStore.cpp
        )

target_sources(TestUnitConverter
    PRIVATE
        test/TestUnitConverter.cpp
        src/Audio/UnitConverter.cpp)
# `target_compile_definitions` adds some preprocessor definitions to our target. In a Projucer
# project, these might be passed in the 'Preprocessor Definitions' field. JUCE modules also make use
# of compile definitions to switch certain features on/off, so if there's a particular feature you
# need that's not on by default, check the module header for the correct flag to set here. These
# definitions will be visible both to your code, and also the JUCE module code, so for new
# definitions, pick unique names that are unlikely to collide! This is a standard CMake command.

target_compile_definitions(Kholors
    PRIVATE
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_DISPLAY_SPLASH_SCREEN=0 # added to remove splash screen as we're using gpl
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Kholors,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Kholors,JUCE_VERSION>")

# TODO: may remove this if cherry picking TestAudioLib linked libs
target_compile_definitions(TestAudioLib
    PRIVATE
        WITH_TESTING
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_DISPLAY_SPLASH_SCREEN=0 # added to remove splash screen as we're using gpl
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Kholors,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Kholors,JUCE_VERSION>")

# TODO: may remove this if cherry picking TestAudioLib linked libs
target_compile_definitions(TestSamplePlayer
    PRIVATE
        WITH_TESTING
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_DISPLAY_SPLASH_SCREEN=0 # added to remove splash screen as we're using gpl
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Kholors,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Kholors,JUCE_VERSION>")

target_compile_definitions(TestUnitConverter
    PRIVATE
        WITH_TESTING
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_DISPLAY_SPLASH_SCREEN=0 # added to remove splash screen as we're using gpl
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Kholors,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Kholors,JUCE_VERSION>")

target_compile_definitions(TestTextureManager
    PRIVATE
        WITH_TESTING
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_DISPLAY_SPLASH_SCREEN=0 # added to remove splash screen as we're using gpl
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Kholors,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Kholors,JUCE_VERSION>")

target_compile_definitions(TestGitWrapper
    PRIVATE
        WITH_TESTING
        # JUCE_WEB_BROWSER and JUCE_USE_CURL would be on by default, but you might not need them.
        JUCE_WEB_BROWSER=0  # If you remove this, add `NEEDS_WEB_BROWSER TRUE` to the `juce_add_gui_app` call
        JUCE_USE_CURL=0     # If you remove this, add `NEEDS_CURL TRUE` to the `juce_add_gui_app` call
        JUCE_DISPLAY_SPLASH_SCREEN=0 # added to remove splash screen as we're using gpl
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:Kholors,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:Kholors,JUCE_VERSION>")

# If your target needs extra binary assets, you can add them here. The first argument is the name of
# a new static library target that will include all the binary resources. There is an optional
# `NAMESPACE` argument that can specify the namespace of the generated binary data class. Finally,
# the SOURCES argument should be followed by a list of source files that should be built into the
# static library. These source files can be of any kind (wav data, images, fonts, icons etc.).
# Conversion to binary-data will happen when your target is built.

# juce_add_binary_data(GuiAppData SOURCES ...)

# `target_link_libraries` links libraries and JUCE modules to other libraries or executables. Here,
# we're linking our executable target to the `juce::juce_gui_extra` module. Inter-module
# dependencies are resolved automatically, so `juce_core`, `juce_events` and so on will also be
# linked automatically. If we'd generated a binary data target above, we would need to link to it
# here too. This is a standard CMake command.

target_link_libraries(Kholors
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_basics
        juce::juce_opengl
        yaml-cpp
        nlohmann_json::nlohmann_json
        git2
        ssl
        crypto
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

target_link_libraries(TestConfig PRIVATE yaml-cpp)

# TODO: cherry pick TestGitWrapper linked libs to remove unnecessary bloat
target_link_libraries(TestGitWrapper
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_basics
        yaml-cpp
        git2
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)


# TODO: cherry pick TestAudioLib linked libs to remove unnecessary bloat
target_link_libraries(TestAudioLib
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# TODO: cherry pick TestSamplePlayer linked libs to remove unnecessary bloat
target_link_libraries(TestSamplePlayer
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_basics
        ssl
        crypto
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# TODO: cherry pick TestUnitConverter linked libs to remove unnecessary bloat
target_link_libraries(TestUnitConverter
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

# TODO: cherry pick TestUnitConverter linked libs to remove unnecessary bloat
target_link_libraries(TestTextureManager
    PRIVATE
        # GuiAppData            # If we'd created a binary data target, we'd link to it here
        juce::juce_gui_extra
        juce::juce_audio_utils
        juce::juce_dsp
        juce::juce_audio_basics
        ssl
        crypto
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)