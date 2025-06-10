option(NUI_FETCH_TRAITS "Fetch nui traits" ON)
set(NUI_TRAITS_GIT_REPOSITORY "https://github.com/NuiCpp/traits.git" CACHE STRING "nui traits git repository")
set(NUI_TRAITS_GIT_TAG "9a9de09ac0eaab4f9e0869a2f4004a5e3e6915ca" CACHE STRING "nui traits git tag")

if(NUI_FETCH_TRAITS)
    include(FetchContent)
    FetchContent_Declare(
        traits-library
        GIT_REPOSITORY ${NUI_TRAITS_GIT_REPOSITORY}
        GIT_TAG        ${NUI_TRAITS_GIT_TAG}
    )

    FetchContent_MakeAvailable(traits-library)
endif()