cmake_minimum_required (VERSION 3.10)

# Enable Hot Reload for MSVC compilers if supported.
if (POLICY CMP0141)
  cmake_policy(SET CMP0141 NEW)
  set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<IF:$<AND:$<C_COMPILER_ID:MSVC>,$<CXX_COMPILER_ID:MSVC>>,$<$<CONFIG:Debug,RelWithDebInfo>:EditAndContinue>,$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>>")
endif()

set(MSVC_RUNTIME_LIBRARY "MultiThreaded<$<CONFIG:Debug>:Debug>")

project ("mi_nobl_ksu")

if(MSVC)
    add_compile_options(/utf-8 /Oi /Gy /GS- /GR-)
    add_link_options(/LTCG /OPT:REF /OPT:ICF /RELEASE /DYNAMICBASE:NO)
endif()

add_executable (mi_nobl_ksu "mi_nobl_ksu.cpp" )

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET mi_nobl_ksu PROPERTY CXX_STANDARD 23)
endif()
