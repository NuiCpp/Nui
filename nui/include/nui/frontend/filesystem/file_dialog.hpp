#pragma once

#include <nui/filesystem/file_dialog_options.hpp>

#include <functional>

namespace Nui::FileDialog
{
    void showOpenDialog(
        OpenDialogOptions const& options,
        std::function<void(std::optional<std::vector<std::filesystem::path>>)> onResult);
    void showDirectoryDialog(
        DirectoryDialogOptions const& options,
        std::function<void(std::optional<std::vector<std::filesystem::path>>)> onResult);
    void showSaveDialog(
        SaveDialogOptions const& options,
        std::function<void(std::optional<std::filesystem::path>)> onResult);
}