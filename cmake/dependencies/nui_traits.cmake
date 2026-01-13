option(NUI_FETCH_TRAITS "Fetch nui traits" ON)
set(NUI_TRAITS_GIT_REPOSITORY "https://github.com/NuiCpp/traits.git" CACHE STRING "nui traits git repository")
set(NUI_TRAITS_GIT_TAG "v1.1.0" CACHE STRING "nui traits git tag")

if(NUI_FETCH_TRAITS)
    include(FetchContent)
    FetchContent_Declare(
        traits-library
        GIT_REPOSITORY ${NUI_TRAITS_GIT_REPOSITORY}
        GIT_TAG        ${NUI_TRAITS_GIT_TAG}
    )

    FetchContent_MakeAvailable(traits-library)
endif()