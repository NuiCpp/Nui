#include <nui/frontend/filesystem/file.hpp>

#include <nui/frontend/rpc_client.hpp>
#include <nui/frontend/utility/val_conversion.hpp>

#include <cstdint>

namespace Nui
{
    // #####################################################################################################################
    AsyncFile::AsyncFile(int32_t id)
        : fileId_{id}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    AsyncFile::~AsyncFile()
    {
        if (fileId_ != -1)
        {
            RpcClient::getRemoteCallable("Nui::closeFile")(fileId_);
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void AsyncFile::tellg(std::function<void(int32_t)> cb) const
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::tellg", std::move(cb))(fileId_);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void AsyncFile::tellp(std::function<void(int32_t)> cb) const
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::tellp", std::move(cb))(fileId_);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void AsyncFile::seekg(int32_t pos, std::function<void()> cb, std::ios_base::seekdir dir)
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::seekg", std::move(cb))(
            fileId_, pos, static_cast<int32_t>(dir));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void AsyncFile::seekp(int32_t pos, std::function<void()> cb, std::ios_base::seekdir dir)
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::seekp", std::move(cb))(
            fileId_, pos, static_cast<int32_t>(dir));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void AsyncFile::read(int32_t size, std::function<void(std::string&&)> cb)
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::read", std::move(cb))(fileId_, size);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void AsyncFile::readAll(std::function<void(std::string&&)> cb)
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::readAll", std::move(cb))(fileId_);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void AsyncFile::write(std::string const& data, std::function<void()> cb)
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::write", std::move(cb))(fileId_, data);
    }
    // #####################################################################################################################
    void
    openFile(char const* filename, std::ios_base::openmode mode, std::function<void(std::optional<AsyncFile>&&)> onOpen)
    {
        openFile(std::string{filename}, mode, std::move(onOpen));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void openFile(
        std::string const& filename,
        std::ios_base::openmode mode,
        std::function<void(std::optional<AsyncFile>&&)> onOpen)
    {
        RpcClient::getRemoteCallableWithBackChannel("Nui::openFile", [onOpen = std::move(onOpen)](Nui::val response) {
            bool success{false};
            convertFromVal(response["success"], success);
            if (!success)
            {
                onOpen(std::nullopt);
                return;
            }

            int32_t id;
            convertFromVal(response["id"], id);
            onOpen(AsyncFile{id});
        })(filename, static_cast<int32_t>(mode));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void openFile(
        std::filesystem::path const& filename,
        std::ios_base::openmode mode,
        std::function<void(std::optional<AsyncFile>&&)> onOpen)
    {
        openFile(filename.string(), mode, std::move(onOpen));
    }
    // #####################################################################################################################
}