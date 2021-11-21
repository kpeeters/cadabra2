
# Policy settings for CMake to resolve ambiguities.

if (POLICY CMP0042)
  cmake_policy(SET CMP0042 NEW)
endif()
if (POLICY CMP0054)
  cmake_policy(SET CMP0054 NEW)
endif()
if (POLICY CMP0127)
  cmake_policy(SET CMP0127 NEW)
endif()