# Tests. They need libfftw3f-3.dll: set DTBLKFIX_FFTW_DIR to the folder that holds it.
set(presets ${PROJECT_SOURCE_DIR}/resources/stereo_presets.txt)

function(dtblkfix_test_exe name engine)
  add_executable(${name} ${ARGN})
  target_link_libraries(${name} PRIVATE ${engine})
  dtblkfix_engine_options(${name})
endfunction()

dtblkfix_test_exe(dtblkfix_render dtblkfix_engine render.cpp)
dtblkfix_test_exe(dtblkfix_sync_test dtblkfix_engine sync_test.cpp)
add_executable(dtblkfix_compare compare.cpp)

# screenshots of the editor (not run as a test, it needs a desktop)
dtblkfix_test_exe(dtblkfix_gui_snapshot dtblkfix_engine gui_snapshot.cpp)

# plugin state (with size & skin) and editor through the wrapper's engine side
add_executable(dtblkfix_bridge_test bridge_test.cpp)
target_link_libraries(dtblkfix_bridge_test PRIVATE dtblkfix_bridge dtblkfix_engine)
dtblkfix_copy_resources(dtblkfix_bridge_test $<TARGET_FILE_DIR:dtblkfix_bridge_test>/dtblkfx)
if(DTBLKFIX_FFTW_DIR)
  add_test(NAME state_and_editor COMMAND dtblkfix_bridge_test)
endif()

if(DTBLKFIX_FFTW_DIR)
  add_test(NAME sync COMMAND dtblkfix_sync_test ${DTBLKFIX_FFTW_DIR} ${presets})
endif()

# bit-identical renders against the original engine
if(DTBLKFIX_REFERENCE_DIR)
  dtblkfix_add_engine(dtblkfix_engine_ref ${DTBLKFIX_REFERENCE_DIR})
  dtblkfix_test_exe(dtblkfix_render_ref dtblkfix_engine_ref render.cpp)
  if(DTBLKFIX_FFTW_DIR)
    set(out ${CMAKE_CURRENT_BINARY_DIR}/renders)
    file(MAKE_DIRECTORY ${out})
    # each case: name and render options
    foreach(case "sync_off:--sync-off" "sync_off_host:--sync-off;--host"
                 "sync_off_buf64:--sync-off;--buf;64" "as_is:" "as_is_host:--host")
      string(REPLACE ":" ";" case "${case}")
      list(GET case 0 case_name)
      list(SUBLIST case 1 -1 case_args)
      add_test(NAME render_ref_${case_name}
               COMMAND dtblkfix_render_ref ${DTBLKFIX_FFTW_DIR} ${presets}
                       ${out}/ref_${case_name}.bin ${case_args})
      add_test(NAME render_new_${case_name}
               COMMAND dtblkfix_render ${DTBLKFIX_FFTW_DIR} ${presets}
                       ${out}/new_${case_name}.bin ${case_args})
      set(allow "")
      if(case_name MATCHES "^as_is")
        set(allow --allow-sync)
      endif()
      add_test(NAME identical_${case_name}
               COMMAND dtblkfix_compare ${out}/ref_${case_name}.bin ${out}/new_${case_name}.bin
                       ${allow})
      set_tests_properties(identical_${case_name} PROPERTIES
                           DEPENDS "render_ref_${case_name};render_new_${case_name}")
    endforeach()
  endif()
endif()
