# The "dtblkfx" folder that the plugin loads from: FFTW, presets, images and the skins (made from
# resources/ by tools/make_skins.py). It's staged in the build folder and copied next to each
# binary that needs it with dtblkfix_copy_resources().

find_package(Python3 REQUIRED COMPONENTS Interpreter)

set(DTBLKFIX_STAGE ${CMAKE_BINARY_DIR}/dtblkfx)
file(GLOB DTBLKFIX_IMAGES ${PROJECT_SOURCE_DIR}/resources/stereo_*.png)
add_custom_command(OUTPUT ${DTBLKFIX_STAGE}/skins/.done
  COMMAND ${CMAKE_COMMAND} -E make_directory ${DTBLKFIX_STAGE}
  COMMAND ${CMAKE_COMMAND} -E copy ${DTBLKFIX_IMAGES}
          ${PROJECT_SOURCE_DIR}/resources/stereo_presets.txt ${DTBLKFIX_STAGE}
  COMMAND Python3::Interpreter ${PROJECT_SOURCE_DIR}/tools/make_skins.py
          ${PROJECT_SOURCE_DIR}/resources ${DTBLKFIX_STAGE}/skins
  COMMAND ${CMAKE_COMMAND} -E touch ${DTBLKFIX_STAGE}/skins/.done
  DEPENDS ${DTBLKFIX_IMAGES} ${PROJECT_SOURCE_DIR}/resources/stereo_presets.txt
          ${PROJECT_SOURCE_DIR}/tools/make_skins.py
  COMMENT "Making the skins")

set(fftw_copy "")
if(DTBLKFIX_FFTW_DIR)
  set(fftw_copy COMMAND ${CMAKE_COMMAND} -E copy ${DTBLKFIX_FFTW_DIR}/libfftw3f-3.dll
                ${DTBLKFIX_STAGE})
endif()
add_custom_target(dtblkfix_resources ALL ${fftw_copy} DEPENDS ${DTBLKFIX_STAGE}/skins/.done)

function(dtblkfix_copy_resources target dest)
  add_dependencies(${target} dtblkfix_resources)
  add_custom_command(TARGET ${target} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E remove_directory ${dest}
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${DTBLKFIX_STAGE} ${dest}
    COMMAND ${CMAKE_COMMAND} -E remove ${dest}/skins/.done)
endfunction()
