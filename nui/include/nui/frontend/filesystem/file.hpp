#pragma once

#include <filesystem>
#include <string>
#include <functional>
#include <optional>
#include <cstdint>
#include <ios>

namespace Nui
{
    /**
     * @brief Do note that the use of this class is inefficient. Prefer implementing your file logic on the backend and
     * pass functions with broader scope through to the frontend.
     */
    class AsyncFile
    {
      public:
        friend void openFile(
            char const* filename,
            std::ios_base::openmode mode,
            std::function<void(std::optional<AsyncFile>&&)> onOpen);
        friend void openFile(
            std::string const& filename,
            std::ios_base::openmode mode,
            std::function<void(std::optional<AsyncFile>&&)> onOpen);
        friend void openFile(
            std::filesystem::path const& filename,
            std::ios_base::openmode mode,
            std::function<void(std::optional<AsyncFile>&&)> onOpen);

      public:
        ~AsyncFile();

        void tellg(std::function<void(int32_t)> cb) const;
        void tellp(std::function<void(int32_t)> cb) const;
        void seekg(int32_t pos, std::function<void()> cb, std::ios_base::seekdir dir = std::ios_base::beg);
        void seekp(int32_t pos, std::function<void()> cb, std::ios_base::seekdir dir = std::ios_base::beg);

        void read(int32_t size, std::function<void(std::string&&)> cb);
        void readAll(std::function<void(std::string&&)> cb);
        void write(std::string const& data, std::function<void()> cb);

      private:
        AsyncFile(int32_t id);

      private:
        int32_t fileId_;
    };
    void openFile(
        char const* filename,
        std::ios_base::openmode mode,
        std::function<void(std::optional<AsyncFile>&&)> onOpen);
    void openFile(
        std::string const& filename,
        std::ios_base::openmode mode,
        std::function<void(std::optional<AsyncFile>&&)> onOpen);
    void openFile(
        std::filesystem::path const& filename,
        std::ios_base::openmode mode,
        std::function<void(std::optional<AsyncFile>&&)> onOpen);
}