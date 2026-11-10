set(PLATINUM_GAMBATTE "" CACHE PATH "Path to the pinned gambatte-libretro source tree")
set(PLATINUM_TEST_ROM "" CACHE FILEPATH "Optional locally built GMUD_GB ROM for the smoke test")
if(NOT EXISTS "${PLATINUM_GAMBATTE}/Makefile.common")
  message(FATAL_ERROR "Set PLATINUM_GAMBATTE to the source tree described in docs/PLATINUM.md")
endif()

set(core "${PLATINUM_GAMBATTE}/libgambatte")
set(common "${core}/libretro-common")
file(GLOB gambatte_sources "${core}/src/*.cpp" "${core}/src/mem/*.cpp"
     "${core}/src/sound/*.cpp" "${core}/src/video/*.cpp")
add_library(platinum_core OBJECT kindle/platinum_core.cpp ${gambatte_sources}
  "${core}/libretro/libretro.cpp"
  "${core}/libretro/gambatte_log.c"
  "${core}/libretro/blipper.c"
  "${core}/libretro/cc_resampler.c"
  "${common}/compat/compat_posix_string.c"
  "${common}/compat/compat_snprintf.c"
  "${common}/compat/compat_strcasestr.c"
  "${common}/compat/compat_strl.c"
  "${common}/compat/fopen_utf8.c"
  "${common}/encodings/encoding_utf.c"
  "${common}/file/file_path.c"
  "${common}/file/file_path_io.c"
  "${common}/streams/file_stream.c"
  "${common}/streams/file_stream_transforms.c"
  "${common}/string/stdstring.c"
  "${common}/time/rtime.c"
  "${common}/vfs/vfs_implementation.c")
target_include_directories(platinum_core PUBLIC kindle PRIVATE
  "${core}/src" "${core}/include" "${core}/libretro" "${common}/include"
  "${PLATINUM_GAMBATTE}/common" "${PLATINUM_GAMBATTE}/common/resample")
target_compile_definitions(platinum_core PRIVATE
  __LIBRETRO__ HAVE_STDINT_H HAVE_INTTYPES_H CC_RESAMPLER_NO_HIGHPASS VIDEO_RGB565)

if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  add_executable(platinum_hero kindle/platinum.cpp kindle/fb.cpp kindle/touch.cpp)
  target_compile_options(platinum_hero PRIVATE -Wall -Wextra)
  target_link_libraries(platinum_hero PRIVATE platinum_core rt)
endif()

include(CTest)
if(BUILD_TESTING AND NOT CMAKE_CROSSCOMPILING)
  add_executable(test_platinum tests/test_platinum.cpp)
  target_include_directories(test_platinum PRIVATE "${common}/include")
  target_link_libraries(test_platinum PRIVATE platinum_core)
  add_test(NAME platinum_display_bounds COMMAND test_platinum)
  if(PLATINUM_TEST_ROM)
    add_test(NAME platinum_gmud_boot_and_input COMMAND test_platinum "${PLATINUM_TEST_ROM}")
  endif()
endif()
