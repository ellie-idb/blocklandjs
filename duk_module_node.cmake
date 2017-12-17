set(duknode ${CMAKE_CURRENT_LIST_DIR}/lib/duk_module_node)

include_directories(
  ${duknode}/src
)

add_library(duknode STATIC ${duknode}/src/duk_module_node.c)

if("${CMAKE_SYSTEM_NAME}" MATCHES "Linux")
  target_link_libraries(DUKNODE
    m dl rt
  )
endif()