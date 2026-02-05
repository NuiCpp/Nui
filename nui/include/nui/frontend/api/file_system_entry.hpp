#pragma once

#include <nui/frontend/val_wrapper.hpp>

#include <string>

namespace Nui::WebApi
{
    class FileSystemEntry : public ValWrapper
    {
      public:
        explicit FileSystemEntry(Nui::val fileSystemEntry);

        /**
         * @brief The FileSystem object associated with the entry.
         */
        Nui::val filesystem() const;

        /**
         * @brief A string which provides the full, absolute path from the file system's root to the entry; it can also
         * be thought of as a path which is relative to the root directory, prepended with a "/" character.
         */
        std::string fullPath() const;

        /**
         * @brief A boolean value which is true if the entry represents a file; otherwise, it's false.
         */
        bool isFile() const;

        /**
         * @brief A boolean value which is true if the entry represents a directory; otherwise, it's false.
         */
        bool isDirectory() const;

        /**
         * @brief The name of the entry (not including the path leading to it).
         */
        std::string name() const;
    };
}