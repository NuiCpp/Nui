project(parcel NONE)

execute_process(
    COMMAND npm install parcel --save-dev
    WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/module_build
)

