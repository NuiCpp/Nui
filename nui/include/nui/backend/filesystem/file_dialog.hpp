#pragma once

#include <nui/filesystem/file_dialog_options.hpp>

namespace Nui::FileDialog
{
    std::optional<std::vector<std::filesystem::path>> showOpenDialog(OpenDialogOptions const& options);
    std::optional<std::vector<std::filesystem::path>> showDirectoryDialog(DirectoryDialogOptions const& options);
    std::optional<std::filesystem::path> showSaveDialog(SaveDialogOptions const& options);
}