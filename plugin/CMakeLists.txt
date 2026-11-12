# DtBlkFix as a VST3 and a standalone app, with JUCE 8. Needs DTBLKFIX_JUCE_DIR (a JUCE checkout).

if(NOT DTBLKFIX_JUCE_DIR)
  message(FATAL_ERROR "Set DTBLKFIX_JUCE_DIR to a JUCE 8 checkout (or DTBLKFIX_PLUGIN=OFF)")
endif()
add_subdirectory(${DTBLKFIX_JUCE_DIR} ${CMAKE_BINARY_DIR}/JUCE EXCLUDE_FROM_ALL)

juce_add_plugin(DtBlkFix
  PRODUCT_NAME "DtBlkFix"
  VERSION ${PROJECT_VERSION}
  COMPANY_NAME "kadspace"
  BUNDLE_ID "com.kadspace.dtblkfix"
  PLUGIN_MANUFACTURER_CODE Kdsp
  PLUGIN_CODE Dbfx
  FORMATS VST3 Standalone
  IS_SYNTH FALSE
  NEEDS_MIDI_INPUT FALSE
  NEEDS_MIDI_OUTPUT FALSE
  IS_MIDI_EFFECT FALSE
  EDITOR_WANTS_KEYBOARD_FOCUS FALSE
  VST3_CATEGORIES Fx
  VST3_AUTO_MANIFEST FALSE
  COPY_PLUGIN_AFTER_BUILD FALSE)

target_sources(DtBlkFix PRIVATE PluginProcessor.cpp PluginEditor.cpp)
set(DTBLKFIX_JUCE_DEFS JUCE_WEB_BROWSER=0 JUCE_USE_CURL=0 JUCE_VST3_CAN_REPLACE_VST2=0
    JUCE_REPORT_APP_USAGE=0 JUCE_STANDALONE_FILTER_WINDOW_USE_KIOSK_MODE=0
    JUCE_GUI_BASICS_INCLUDE_SCOPED_THREAD_DPI_AWARENESS_SETTER=1)
target_compile_definitions(DtBlkFix PUBLIC ${DTBLKFIX_JUCE_DEFS})
target_link_libraries(DtBlkFix
  PRIVATE dtblkfix_bridge juce::juce_audio_utils juce::juce_gui_extra
  PUBLIC juce::juce_recommended_config_flags juce::juce_recommended_warning_flags)

# its "dtblkfx" folder: in the VST3 bundle (Contents/Resources) and next to the standalone app
dtblkfix_copy_resources(DtBlkFix_VST3 "$<TARGET_FILE_DIR:DtBlkFix_VST3>/../Resources/dtblkfx")
dtblkfix_copy_resources(DtBlkFix_Standalone "$<TARGET_FILE_DIR:DtBlkFix_Standalone>/dtblkfx")

# the single-file VST3: the "dtblkfx" folder and the licenses, packed into the plugin as a resource
# (tools/pack_resources.py). The plugin unpacks it to %LOCALAPPDATA%\DtBlkFix\<version> when there's
# no "dtblkfx" folder beside it or in its bundle.
if(DTBLKFIX_FFTW_DIR)
  set(pack ${CMAKE_CURRENT_BINARY_DIR}/dtblkfix_resources.pak)
  set(licenses
    licenses/DtBlkFix-LICENSE.txt=${PROJECT_SOURCE_DIR}/LICENSE
    licenses/THIRD_PARTY_NOTICES.md=${PROJECT_SOURCE_DIR}/THIRD_PARTY_NOTICES.md
    licenses/FFTW-COPYING.txt=${PROJECT_SOURCE_DIR}/fftw/COPYING
    licenses/JUCE-LICENSE.md=${DTBLKFIX_JUCE_DIR}/LICENSE.md
    licenses/VST3_SDK-LICENSE.txt=${DTBLKFIX_JUCE_DIR}/modules/juce_audio_processors_headless/format_types/VST3_SDK/LICENSE.txt)
  set(license_files ${licenses})
  list(TRANSFORM license_files REPLACE "^[^=]*=" "")
  add_custom_command(OUTPUT ${pack}
    COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/pack_resources.py ${pack}
            dtblkfx=${DTBLKFIX_STAGE}
            dtblkfx/libfftw3f-3.dll=${DTBLKFIX_FFTW_DIR}/libfftw3f-3.dll ${licenses}
    DEPENDS ${PROJECT_SOURCE_DIR}/tools/pack_resources.py ${DTBLKFIX_STAGE}/skins/.done
            ${DTBLKFIX_FFTW_DIR}/libfftw3f-3.dll ${license_files}
    COMMENT "Packing the resources into the plugin")
  add_custom_target(dtblkfix_pack DEPENDS ${pack})
  add_dependencies(dtblkfix_pack dtblkfix_resources)

  set(DTBLKFIX_PACK ${pack})
  configure_file(resources.rc.in ${CMAKE_CURRENT_BINARY_DIR}/resources.rc @ONLY)
  set_source_files_properties(${CMAKE_CURRENT_BINARY_DIR}/resources.rc PROPERTIES
    OBJECT_DEPENDS ${pack})
  target_sources(DtBlkFix_VST3 PRIVATE ${CMAKE_CURRENT_BINARY_DIR}/resources.rc)
  add_dependencies(DtBlkFix_VST3 dtblkfix_pack)
endif()

# the processor, as a console app (tests/plugin_test.cpp)
if(DTBLKFIX_TESTS)
  juce_add_console_app(dtblkfix_plugin_test PRODUCT_NAME "dtblkfix_plugin_test")
  target_sources(dtblkfix_plugin_test PRIVATE
    ${PROJECT_SOURCE_DIR}/tests/plugin_test.cpp PluginProcessor.cpp PluginEditor.cpp)
  target_include_directories(dtblkfix_plugin_test PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
  target_compile_definitions(dtblkfix_plugin_test PRIVATE ${DTBLKFIX_JUCE_DEFS}
    JucePlugin_Name="DtBlkFix")
  target_link_libraries(dtblkfix_plugin_test PRIVATE
    dtblkfix_bridge juce::juce_audio_utils juce::juce_gui_extra
    juce::juce_recommended_config_flags)
  dtblkfix_copy_resources(dtblkfix_plugin_test "$<TARGET_FILE_DIR:dtblkfix_plugin_test>/dtblkfx")
  if(DTBLKFIX_FFTW_DIR)
    add_test(NAME plugin_state COMMAND dtblkfix_plugin_test)

    # the single-file VST3 in a host (tests/host_test.cpp): unpacking, then using what's unpacked
    juce_add_console_app(dtblkfix_host_test PRODUCT_NAME "dtblkfix_host_test")
    target_sources(dtblkfix_host_test PRIVATE ${PROJECT_SOURCE_DIR}/tests/host_test.cpp)
    target_compile_definitions(dtblkfix_host_test PRIVATE ${DTBLKFIX_JUCE_DEFS}
      JUCE_PLUGINHOST_VST3=1 DTBLKFIX_VERSION="${PROJECT_VERSION}")
    target_link_libraries(dtblkfix_host_test PRIVATE
      juce::juce_audio_processors juce::juce_recommended_config_flags)
    add_dependencies(dtblkfix_host_test DtBlkFix_VST3)
    set(work ${CMAKE_CURRENT_BINARY_DIR}/host_test)
    add_test(NAME host_unpack
      COMMAND dtblkfix_host_test $<TARGET_FILE:DtBlkFix_VST3> ${work} --fresh)
    add_test(NAME host_reuse COMMAND dtblkfix_host_test $<TARGET_FILE:DtBlkFix_VST3> ${work})
    set_tests_properties(host_reuse PROPERTIES DEPENDS host_unpack)
  endif()
endif()
