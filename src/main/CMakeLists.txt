add_executable(
  app
  main.cpp
  app.cpp
  app.hpp
  probe.cpp
  probe.hpp
)

target_link_libraries(
  app
  PRIVATE
  core
  spdlog
)

set_target_properties(
  app
  PROPERTIES
  RUNTIME_OUTPUT_DIRECTORY
  "${CMAKE_BINARY_DIR}"
)
