# cmake rule for GCC address-sanitizer (tsan).
#
# Make sure gcc 4.8 (or up) is installed and configured.
#
# To use a GCC different from the default version, set the env vars.
# For example:
#
#	  $ export CC=/opt/gcc-4.8.2/bin/gcc
#	  $ export CXX=/opt/gcc-4.8.2/bin/g++
#

# uncomment this to use tsan all the time
#SET(USE_ASAN ON)

if (USE_ASAN)
  execute_process(COMMAND ${CMAKE_C_COMPILER} -dumpversion OUTPUT_VARIABLE
                  GCC_VERSION)
  if (GCC_VERSION VERSION_LESS 4.8)
    message(FATAL_ERROR "address-sanitizer is not supported by GCC ${GCC_VERSION}")
  endif()

  message(STATUS "address-sanitizer powered by GCC ${GCC_VERSION}")

  set(ASAN_C_FLAGS "-fsanitize=address -fPIE")
  set(ASAN_CXX_FLAGS "-fsanitize=address -fPIE")

  set(ASAN_EXE_LINKER_FLAGS "-fsanitize=address -pie")
  set(ASAN_SHARED_LINKER_FLAGS "-fsanitize=address -pie")

  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${ASAN_C_FLAGS}")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ASAN_CXX_FLAGS}")

  set(CMAKE_EXE_LINKER_FLAGS
      "${CMAKE_EXE_LINKER_FLAGS} ${ASAN_EXE_LINKER_FLAGS}")
  set(CMAKE_SHARED_LINKER_FLAGS
      "${CMAKE_SHARED_LINKER_FLAGS} ${ASAN_SHARED_LINKER_FLAGS}")
endif()

# vim:expandtab:shiftwidth=2:tabstop=2:
