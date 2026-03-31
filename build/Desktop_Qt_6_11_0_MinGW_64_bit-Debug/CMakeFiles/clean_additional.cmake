# Additional clean files
cmake_minimum_required(VERSION 3.16)

if("${CONFIG}" STREQUAL "" OR "${CONFIG}" STREQUAL "Debug")
  file(REMOVE_RECURSE
  "CMakeFiles\\OS_autogen.dir\\AutogenUsed.txt"
  "CMakeFiles\\OS_autogen.dir\\ParseCache.txt"
  "OS_autogen"
  )
endif()
