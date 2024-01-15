cmake_minimum_required(VERSION 3.18)

string(REPLACE " " ";" LIST_SHADER_FILES ${PARAM_SHADER_FILES})

foreach(SHADER_SOURCE ${LIST_SHADER_FILES})
	message("Compile:  ${SHADER_SOURCE} into ${SHADER_SOURCE}.spv with flags: ${PARAM_SHADER_FLAGS}")
	execute_process(COMMAND ${Vulkan_GLSLC_EXECUTABLE} "${SHADER_SOURCE}" ${PARAM_SHADER_FLAGS} -o "${SHADER_SOURCE}.spv"
	WORKING_DIRECTORY "${PARAM_WORKING_DIR}")
endforeach()