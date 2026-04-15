# CMake generated Testfile for 
# Source directory: /Users/riyananosh/waydroid-xr-stubs/host
# Build directory: /Users/riyananosh/waydroid-xr-stubs/host/build
# 
# This file includes the relevant testing commands required for 
# testing this directory and lists subdirectories to be tested as well.
add_test([=[gpu_integration_tests]=] "/Users/riyananosh/waydroid-xr-stubs/host/build/test_gpu_integration")
set_tests_properties([=[gpu_integration_tests]=] PROPERTIES  _BACKTRACE_TRIPLES "/Users/riyananosh/waydroid-xr-stubs/host/CMakeLists.txt;128;add_test;/Users/riyananosh/waydroid-xr-stubs/host/CMakeLists.txt;0;")
subdirs("_deps/openxr-build")
