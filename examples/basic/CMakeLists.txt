cmake_minimum_required(VERSION 3.21)

project(nui-template VERSION 0.1.0)

include (${CMAKE_CURRENT_LIST_DIR}/_cmake/common_options.cmake)

# Add executable (sources are added later, depending on frontend/backend)
add_executable(basic-example)
set_common_options(basic-example)

set(PREJS_FILE ${CMAKE_CURRENT_LIST_DIR}/frontend/source/frontend/js/module.js)

if (EMSCRIPTEN)  
    add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/frontend/source/frontend)
else()
	add_subdirectory(${CMAKE_CURRENT_LIST_DIR}/backend/source/backend)
endif()