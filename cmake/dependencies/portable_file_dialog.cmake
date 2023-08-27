option(NUI_FETCH_PORTABLE_FILE_DIALOG "Fetch portable-file-dialogs" ON)
set(NUI_PORTABLE_FILE_DIALOG_REPOSITORY "https://github.com/samhocevar/portable-file-dialogs.git" CACHE STRING "Repository of portable-file-dialogs")
set(NUI_PORTABLE_FILE_DIALOG_TAG "7f852d88a480020d7f91957cbcefe514fc95000c" CACHE STRING "Tag of portable-file-dialogs")

if(NUI_FETCH_PORTABLE_FILE_DIALOG)
    include(FetchContent)
    FetchContent_Declare(
        portable_file_dialogs
        GIT_REPOSITORY ${NUI_PORTABLE_FILE_DIALOG_REPOSITORY}
        GIT_TAG        ${NUI_PORTABLE_FILE_DIALOG_TAG}
    )

    FetchContent_MakeAvailable(portable_file_dialogs)
endif()