#include <nui/backend/filesystem/file_dialog.hpp>

#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Weverything"
#define PFD_HAS_IFILEDIALOG 1
#include <portable-file-dialogs.h>
#pragma clang diagnostic pop

#include <nui/backend/filesystem/special_paths.hpp>

#include <iostream>

namespace Nui::FileDialog
{
    // #####################################################################################################################
    namespace
    {
        //---------------------------------------------------------------------------------------------------------------------
        std::vector<std::string> flattenFilters(std::vector<Filter> const& filters)
        {
            std::vector<std::string> flattenedFilters;
            if (filters.empty())
            {
                flattenedFilters.push_back("All files");
                flattenedFilters.push_back("*");
            }
            else
            {
                for (auto const& filter : filters)
                {
                    flattenedFilters.push_back(filter.name);
                    std::string combinedMask;
                    for (auto const& mask : filter.masks)
                    {
                        combinedMask += mask;
                        combinedMask += " ";
                    }
                    combinedMask.pop_back();
                    flattenedFilters.push_back(combinedMask);
                }
            }
            return flattenedFilters;
        }
        //---------------------------------------------------------------------------------------------------------------------
        pfd::opt convertOptions(bool allowMultiSelect, bool forceOverwrite, bool forcePath)
        {
            pfd::opt opts = pfd::opt::none;
            if (allowMultiSelect)
                opts = static_cast<pfd::opt>(static_cast<int>(opts) | static_cast<int>(pfd::opt::multiselect));
            if (forceOverwrite)
                opts = static_cast<pfd::opt>(static_cast<int>(opts) | static_cast<int>(pfd::opt::force_overwrite));
            if (forcePath)
                opts = static_cast<pfd::opt>(static_cast<int>(opts) | static_cast<int>(pfd::opt::force_path));
            return opts;
        }
        //---------------------------------------------------------------------------------------------------------------------
        std::string getTitle(std::optional<std::string> const& givenTitle)
        {
            std::string title = "File Dialog";
            if (givenTitle)
                title = *givenTitle;
            return title;
        }
        //---------------------------------------------------------------------------------------------------------------------
        std::string getDefaultPath(std::optional<std::filesystem::path> const& path)
        {
            std::string defaultPath = resolvePath("%userprofile%").string();
            if (path)
                defaultPath = path->string();
            return defaultPath;
        }
        //---------------------------------------------------------------------------------------------------------------------
        std::optional<std::vector<std::filesystem::path>> getResults(auto& dialog)
        {
            const auto result = dialog.result();
            if (result.empty())
                return std::nullopt;
            std::vector<std::filesystem::path> paths;
            paths.resize(result.size());
            std::transform(result.begin(), result.end(), paths.begin(), [](std::string const& path) {
                return std::filesystem::path(path);
            });
            return paths;
        }
        //---------------------------------------------------------------------------------------------------------------------
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<std::vector<std::filesystem::path>> showOpenDialog(OpenDialogOptions const& options)
    {
        auto dialog = pfd::open_file(
            getTitle(options.title),
            getDefaultPath(options.defaultPath),
            flattenFilters(options.filters),
            convertOptions(options.allowMultiSelect, false, options.forcePath));
        return getResults(dialog);
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<std::vector<std::filesystem::path>> showDirectoryDialog(DirectoryDialogOptions const& options)
    {
        auto dialog = pfd::select_folder(getTitle(options.title), getDefaultPath(options.defaultPath));
        const auto result = dialog.result();
        if (result.empty())
            return std::nullopt;
        return std::vector<std::filesystem::path>{std::filesystem::path(result)};
    }
    //---------------------------------------------------------------------------------------------------------------------
    std::optional<std::filesystem::path> showSaveDialog(SaveDialogOptions const& options)
    {
        auto dialog = pfd::save_file(
            getTitle(options.title),
            getDefaultPath(options.defaultPath),
            flattenFilters(options.filters),
            convertOptions(false, options.forceOverwrite, options.forcePath));
        const auto result = dialog.result();
        if (result.empty())
            return std::nullopt;
        return std::filesystem::path(result);
    }
    // #####################################################################################################################
}