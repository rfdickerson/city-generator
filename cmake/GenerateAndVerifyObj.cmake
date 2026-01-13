if(NOT DEFINED APP)
  message(FATAL_ERROR "APP is required")
endif()
if(NOT DEFINED OUT)
  message(FATAL_ERROR "OUT is required")
endif()

if(NOT IS_ABSOLUTE "${OUT}")
  set(output_path "${CMAKE_CURRENT_BINARY_DIR}/${OUT}")
else()
  set(output_path "${OUT}")
endif()

execute_process(
  COMMAND "${APP}"
  WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}"
  RESULT_VARIABLE run_result
)
if(NOT run_result EQUAL 0)
  message(FATAL_ERROR "buildings exited with code ${run_result}")
endif()

if(NOT EXISTS "${output_path}")
  message(FATAL_ERROR "Expected output not found: ${output_path}")
endif()

file(SIZE "${output_path}" output_size)
if(output_size LESS 1)
  message(FATAL_ERROR "Output file is empty: ${output_path}")
endif()
