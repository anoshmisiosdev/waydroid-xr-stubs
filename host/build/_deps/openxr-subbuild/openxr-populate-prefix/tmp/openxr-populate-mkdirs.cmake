# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file LICENSE.rst or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION ${CMAKE_VERSION}) # this file comes with cmake

# If CMAKE_DISABLE_SOURCE_CHANGES is set to true and the source directory is an
# existing directory in our source tree, calling file(MAKE_DIRECTORY) on it
# would cause a fatal error, even though it would be a no-op.
if(NOT EXISTS "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-src")
  file(MAKE_DIRECTORY "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-src")
endif()
file(MAKE_DIRECTORY
  "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-build"
  "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-subbuild/openxr-populate-prefix"
  "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-subbuild/openxr-populate-prefix/tmp"
  "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-subbuild/openxr-populate-prefix/src/openxr-populate-stamp"
  "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-subbuild/openxr-populate-prefix/src"
  "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-subbuild/openxr-populate-prefix/src/openxr-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-subbuild/openxr-populate-prefix/src/openxr-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/Users/riyananosh/waydroid-xr-stubs/host/build/_deps/openxr-subbuild/openxr-populate-prefix/src/openxr-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
