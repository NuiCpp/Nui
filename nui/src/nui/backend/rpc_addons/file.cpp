#include "file.hpp"

#include <nui/data_structures/selectables_registry.hpp>

#include <string>
#include <fstream>

namespace Nui
{
    namespace Detail
    {
        constexpr static char const* fileStreamStoreId = "FileStreamStore";

        using FileStreamStore = SelectablesRegistry<std::fstream>;

        struct FileStreamStoreCreator
        {
            static void* create()
            {
                return new FileStreamStore();
            }
            static void destroy(void* ptr)
            {
                delete static_cast<FileStreamStore*>(ptr);
            }
        };

        FileStreamStore& getStore(auto& hub)
        {
            return *static_cast<FileStreamStore*>(
                hub.template accessStateStore<FileStreamStoreCreator>(fileStreamStoreId));
        }
    }

    void registerFile(Nui::RpcHub& hub)
    {
        hub.registerFunction(
            "Nui::openFile", [&hub](std::string const& responseId, std::string const& fileName, int openMode) {
                auto& store = Detail::getStore(hub);
                std::fstream stream(fileName, static_cast<std::ios_base::openmode>(openMode));
                if (!stream.is_open())
                {
                    hub.callRemote(responseId, nlohmann::json{{"success", false}});
                    return;
                }

                hub.callRemote(responseId, nlohmann::json{{"success", true}, {"id", store.append(std::move(stream))}});
            });
        hub.registerFunction("Nui::closeFile", [&hub](int32_t id) {
            auto& store = Detail::getStore(hub);
            store.erase(static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id));
        });
        hub.registerFunction("Nui::tellg", [&hub](std::string const& responseId, int32_t id) {
            auto& store = Detail::getStore(hub);
            auto& stream = store[static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id)];
            hub.callRemote(responseId, static_cast<std::streamsize>(stream.tellg()));
        });
        hub.registerFunction("Nui::tellp", [&hub](std::string const& responseId, int32_t id) {
            auto& store = Detail::getStore(hub);
            auto& stream = store[static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id)];
            hub.callRemote(responseId, static_cast<std::streamsize>(stream.tellp()));
        });
        hub.registerFunction("Nui::seekg", [&hub](std::string const& responseId, int32_t id, int32_t pos, int32_t dir) {
            auto& store = Detail::getStore(hub);
            auto& stream = store[static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id)];
            stream.seekg(pos, static_cast<std::ios_base::seekdir>(dir));
            hub.callRemote(responseId);
        });
        hub.registerFunction("Nui::seekp", [&hub](std::string const& responseId, int32_t id, int32_t pos, int32_t dir) {
            auto& store = Detail::getStore(hub);
            auto& stream = store[static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id)];
            stream.seekp(pos, static_cast<std::ios_base::seekdir>(dir));
            hub.callRemote(responseId);
        });
        hub.registerFunction("Nui::read", [&hub](std::string const& responseId, int32_t id, int32_t size) {
            auto& store = Detail::getStore(hub);
            auto& stream = store[static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id)];
            std::string buffer(static_cast<std::size_t>(size), '\0');
            stream.read(buffer.data(), size);
            hub.callRemote(responseId, buffer);
        });
        hub.registerFunction("Nui::readAll", [&hub](std::string const& responseId, int32_t id) {
            auto& store = Detail::getStore(hub);
            auto& stream = store[static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id)];
            std::string buffer;
            stream.seekg(0, std::ios_base::end);
            buffer.resize(static_cast<std::size_t>(stream.tellg()));
            stream.seekg(0, std::ios_base::beg);
            stream.read(buffer.data(), static_cast<std::streamsize>(buffer.size()));
            hub.callRemote(responseId, buffer);
        });
        hub.registerFunction("Nui::write", [&hub](std::string const& responseId, int32_t id, std::string const& data) {
            auto& store = Detail::getStore(hub);
            auto& stream = store[static_cast<Nui::SelectablesRegistry<std::fstream>::IdType>(id)];
            stream.write(data.data(), static_cast<std::streamsize>(data.size()));
            hub.callRemote(responseId);
        });
    }
}