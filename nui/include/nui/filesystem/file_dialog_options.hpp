#pragma once

#include <nui/core.hpp>

#include <optional>
#include <string>
#include <vector>
#include <filesystem>

#ifdef NUI_BACKEND
#    include <nlohmann/json.hpp>
#endif

namespace Nui::FileDialog
{
    struct Filter
    {
        std::string name;
        std::vector<std::string> masks;
    };

// This way aggregate initialization can be done without needing to add extra braces
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define COMMON_DIALOG_OPTIONS() \
    std::optional<std::string> title = std::nullopt; \
    std::optional<std::filesystem::path> defaultPath = std::nullopt; \
    std::vector<Filter> filters = {}; \
    bool forcePath = false

    struct OpenDialogOptions
    {
        COMMON_DIALOG_OPTIONS();

        bool allowMultiSelect = false;
    };

    struct DirectoryDialogOptions
    {
        COMMON_DIALOG_OPTIONS();
    };

    struct SaveDialogOptions
    {
        COMMON_DIALOG_OPTIONS();

        bool forceOverwrite = false;
    };

#ifdef NUI_BACKEND
    NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Filter, name, masks)

    void to_json(nlohmann::json& json, OpenDialogOptions const& options);
    void from_json(nlohmann::json const& json, OpenDialogOptions& options);

    void to_json(nlohmann::json& json, DirectoryDialogOptions const& options);
    void from_json(nlohmann::json const& json, DirectoryDialogOptions& options);

    void to_json(nlohmann::json& json, SaveDialogOptions const& options);
    void from_json(nlohmann::json const& json, SaveDialogOptions& options);
#endif
}