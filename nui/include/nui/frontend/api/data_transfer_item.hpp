#pragma once

#include <nui/frontend/val_wrapper.hpp>
#include <nui/frontend/api/file.hpp>
#include <nui/frontend/api/file_system_entry.hpp>

#include <optional>
#include <string>
#include <functional>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/docs/Web/API/DataTransferItem
     */
    class DataTransferItem : public ValWrapper
    {
      public:
        explicit DataTransferItem(Nui::val dataTransferItem);

        enum class Kind
        {
            Unknown,
            String,
            File
        };
        /**
         * @brief Gets the type of drag-and-drop operation currently selected or sets the operation to a new type.
         */
        Kind kind() const;

        /**
         * @brief The drag data item's type, typically a MIME type.
         */
        std::string type() const;

        /**
         * @brief Returns the File object associated with the drag data item (or null if the drag item is not a file).
         */
        std::optional<File> getAsFile() const;

        /**
         * @brief Returns a Promise that fulfills with a FileSystemFileHandle if the dragged item is a file, or fulfills
         * with a FileSystemDirectoryHandle if the dragged item is a directory.
         */
        Nui::val getAsStringCallback() const;

        /**
         * @brief Invokes the specified callback with the drag data item string as its argument.
         */
        void getAsString(std::function<void(std::optional<std::string> const&)> callback) const;

        /**
         * @brief Returns an object based on FileSystemEntry representing the selected file's entry in its file system.
         * This will generally be either a FileSystemFileEntry or FileSystemDirectoryEntry object.
         */
        std::optional<FileSystemEntry> webkitGetAsEntry() const;
    };
}