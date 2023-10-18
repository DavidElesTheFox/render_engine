cmake_minimum_required(VERSION 3.18)

string(REPLACE " " ";" LIST_SHADER_FILES ${PARAM_SHADER_FILES})

foreach(SHADER_SOURCE ${LIST_SHADER_FILES})
	message("Compile:  ${SHADER_SOURCE} into ${SHADER_SOURCE}.spv")
	execute_process(COMMAND ${Vulkan_GLSLC_EXECUTABLE} "${SHADER_SOURCE}" -o "${SHADER_SOURCE}.spv"
	WORKING_DIRECTORY "${PARAM_WORKING_DIR}")
endforeach()