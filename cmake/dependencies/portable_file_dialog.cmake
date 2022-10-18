project(portable_file_dialogs-git NONE)

# TODO: fork not necessary i think
include(FetchContent)
FetchContent_Declare(
    portable_file_dialogs
    GIT_REPOSITORY https://github.com/5cript/portable-file-dialogs.git
    GIT_TAG        5652fbd0df05f001aa2e92d86c22f762a03c1fd9    
)

FetchContent_MakeAvailable(portable_file_dialogs)