cmake_minimum_required(VERSION 3.22)

project(ModernEdirolSD80 VERSION 1.4.1 LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Point this at a local JUCE checkout to skip the download:
#   cmake -DJUCE_DIR=/path/to/JUCE ..
if (NOT DEFINED JUCE_DIR)
    include(FetchContent)
    FetchContent_Declare(JUCE
        GIT_REPOSITORY https://github.com/juce-framework/JUCE.git
        GIT_TAG        9.0.1
        GIT_SHALLOW    TRUE)
    FetchContent_MakeAvailable(JUCE)
else()
    add_subdirectory("${JUCE_DIR}" JUCE)
endif()

juce_add_plugin(ModernEdirolSD80
    COMPANY_NAME "Crimson Redstone"
    BUNDLE_ID com.crimsonredstone.sd80
    IS_SYNTH TRUE
    NEEDS_MIDI_INPUT TRUE
    NEEDS_MIDI_OUTPUT TRUE
    IS_MIDI_EFFECT FALSE
    EDITOR_WANTS_KEYBOARD_FOCUS TRUE
    COPY_PLUGIN_AFTER_BUILD TRUE
    PLUGIN_MANUFACTURER_CODE Medl
    PLUGIN_CODE Sd80
    FORMATS VST3 AU Standalone
    PRODUCT_NAME "Modern Edirol SD80"
    VST3_CATEGORIES "Instrument"
    AU_MAIN_TYPE kAudioUnitType_MusicDevice
    MICROPHONE_PERMISSION_ENABLED FALSE)

juce_generate_juce_header(ModernEdirolSD80)

target_sources(ModernEdirolSD80
    PRIVATE
        Source/PluginProcessor.cpp
        Source/PluginProcessor.h
        Source/PluginEditor.cpp
        Source/PluginEditor.h
        Source/SD80PatchData.h
        Source/SD80Sysex.h
        Source/SD80LookAndFeel.h
        Source/MidiThrottleQueue.h
        Source/MidiFileImporter.h
        Source/MidiPlayer.h
        Source/CassetteDeck.h
        Source/Skin.h
        Source/ParamLock.h)

target_compile_definitions(ModernEdirolSD80
    PUBLIC
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_VST3_CAN_REPLACE_VST2=0
        JUCE_DISPLAY_SPLASH_SCREEN=0
        JUCE_WASAPI=1
        JUCE_DIRECTSOUND=1)

# ASIO for the Windows standalone (Driver list in Audio/MIDI Settings).
# Steinberg ASIO SDK 2.3.4 is fetched at configure time (GPL-3 since 2025).
# Override with -DASIO_SDK_DIR=/path/to/asiosdk if you already have it.
option(MESD80_ASIO "Enable ASIO in the standalone (Windows)" ON)
if (WIN32 AND MESD80_ASIO)
    set(_asiosdk_inc "")
    if (DEFINED ASIO_SDK_DIR AND EXISTS "${ASIO_SDK_DIR}/common/iasiodrv.h")
        set(_asiosdk_inc "${ASIO_SDK_DIR}/common")
    else()
        include(FetchContent)
        if (POLICY CMP0169)
            cmake_policy(SET CMP0169 OLD)
        endif()
        FetchContent_Declare(asiosdk
            URL https://download.steinberg.net/sdk_downloads/ASIO-SDK_2.3.4_2025-10-15.zip
            DOWNLOAD_EXTRACT_TIMESTAMP TRUE)
        FetchContent_GetProperties(asiosdk)
        if (NOT asiosdk_POPULATED)
            FetchContent_Populate(asiosdk)
        endif()
        if (EXISTS "${asiosdk_SOURCE_DIR}/common/iasiodrv.h")
            set(_asiosdk_inc "${asiosdk_SOURCE_DIR}/common")
        else()
            file(GLOB_RECURSE _asio_hdr "${asiosdk_SOURCE_DIR}/*/iasiodrv.h")
            if (_asio_hdr)
                list(GET _asio_hdr 0 _asio_one)
                get_filename_component(_asiosdk_inc "${_asio_one}" DIRECTORY)
            endif()
        endif()
    endif()
    if (_asiosdk_inc)
        message(STATUS "ASIO SDK headers: ${_asiosdk_inc}")
        target_include_directories(ModernEdirolSD80 PRIVATE "${_asiosdk_inc}")
        target_compile_definitions(ModernEdirolSD80 PUBLIC JUCE_ASIO=1)
    else()
        message(WARNING "ASIO SDK not found. Standalone will ship without ASIO. Set -DASIO_SDK_DIR=...")
    endif()
endif()

target_link_libraries(ModernEdirolSD80
    PRIVATE
        juce::juce_audio_utils
        juce::juce_audio_devices
        juce::juce_audio_processors
        juce::juce_gui_extra
        juce::juce_data_structures
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags)

if (TARGET ModernEdirolSD80_Standalone)
    target_sources(ModernEdirolSD80_Standalone PRIVATE Source/StandaloneApp.cpp)
    target_compile_definitions(ModernEdirolSD80_Standalone PUBLIC JUCE_USE_CUSTOM_PLUGIN_STANDALONE_APP=1)
endif()

# JUCE 9.0.1 does not ship a native CLAP client (no juce_audio_plugin_client_CLAP).
# CLAP still comes from clap-juce-extensions, which supports JUCE 9.
# Disable with -DMESD80_CLAP=OFF.
option(MESD80_CLAP "Build a CLAP via clap-juce-extensions" ON)
if (MESD80_CLAP AND NOT TARGET ModernEdirolSD80_CLAP)
    include(FetchContent)
    if (POLICY CMP0169)
        cmake_policy(SET CMP0169 OLD)
    endif()
    FetchContent_Declare(clap_juce_extensions
        GIT_REPOSITORY https://github.com/free-audio/clap-juce-extensions.git
        GIT_TAG        main
        GIT_SUBMODULES_RECURSE TRUE)
    FetchContent_GetProperties(clap_juce_extensions)
    if (NOT clap_juce_extensions_POPULATED)
        FetchContent_Populate(clap_juce_extensions)
        add_subdirectory("${clap_juce_extensions_SOURCE_DIR}"
                         "${clap_juce_extensions_BINARY_DIR}"
                         EXCLUDE_FROM_ALL)
    endif()
    if (COMMAND clap_juce_extensions_plugin)
        clap_juce_extensions_plugin(
            TARGET ModernEdirolSD80
            CLAP_ID "com.crimsonredstone.sd80"
            CLAP_FEATURES instrument synthesizer)
        message(STATUS "CLAP target: ModernEdirolSD80_CLAP")
    else()
        message(WARNING "clap_juce_extensions_plugin() not found — CLAP skipped")
    endif()
endif()
