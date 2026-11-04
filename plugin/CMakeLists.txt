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
  endif()
endif()
