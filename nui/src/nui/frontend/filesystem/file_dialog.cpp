#include <nui/frontend/filesystem/file_dialog.hpp>
#include <nui/frontend/rpc_client.hpp>
#include <nui/frontend/utility/val_conversion.hpp>
#include <nui/frontend/api/console.hpp>
#include <nui/frontend/event_system/event_context.hpp>

#include <nui/frontend/val.hpp>

#include <algorithm>

namespace Nui::FileDialog
{
    // #####################################################################################################################
    namespace
    {
        template <typename T>
        void convertOptions(Nui::val& opts, T const& options)
        {
            opts.set("title", convertToVal(options.title));
            opts.set("defaultPath", convertToVal(options.defaultPath));
            opts.set("filters", Nui::val::array());
            for (auto const& filter : options.filters)
            {
                Nui::val filterVal = Nui::val::object();
                filterVal.set("name", convertToVal(filter.name));
                filterVal.set("masks", convertToVal(filter.masks));
                opts["filters"].call<void>("push", filterVal);
            }
            opts.set("forcePath", convertToVal(options.forcePath));
        }
    }
    //---------------------------------------------------------------------------------------------------------------------
    void showOpenDialog(
        OpenDialogOptions const& options,
        std::function<void(std::optional<std::vector<std::filesystem::path>>)> onResult)
    {
        Nui::val opts = Nui::val::object();
        convertOptions(opts, options);
        opts.set("allowMultiSelect", options.allowMultiSelect);
        const auto id = RpcClient::registerFunctionOnce([onResult](Nui::val const& param) {
            if (param.typeOf().as<std::string>() == "null")
                onResult(std::nullopt);
            else
            {
                const auto stringPaths = emscripten::vecFromJSArray<std::string>(param);
                std::vector<std::filesystem::path> paths(stringPaths.size());
                std::transform(stringPaths.begin(), stringPaths.end(), paths.begin(), [](std::string const& path) {
                    return std::filesystem::path(path);
                });
                onResult(paths);
            }
            globalEventContext.executeActiveEventsImmediately();
        });
        opts.set("callbackId", id);
        RpcClient::getRemoteCallable("Nui::showOpenDialog")(opts);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void showDirectoryDialog(
        DirectoryDialogOptions const& options,
        std::function<void(std::optional<std::vector<std::filesystem::path>>)> onResult)
    {
        Nui::val opts = Nui::val::object();
        convertOptions(opts, options);
        const auto id = RpcClient::registerFunctionOnce([onResult = std::move(onResult)](Nui::val const& param) {
            if (param.typeOf().as<std::string>() == "null")
                onResult(std::nullopt);
            else
            {
                const auto stringPaths = emscripten::vecFromJSArray<std::string>(param);
                std::vector<std::filesystem::path> paths(stringPaths.size());
                std::transform(stringPaths.begin(), stringPaths.end(), paths.begin(), [](std::string const& path) {
                    return std::filesystem::path(path);
                });
                onResult(paths);
            }
            globalEventContext.executeActiveEventsImmediately();
        });
        opts.set("callbackId", id);
        RpcClient::getRemoteCallable("Nui::showDirectoryDialog")(opts);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void
    showSaveDialog(SaveDialogOptions const& options, std::function<void(std::optional<std::filesystem::path>)> onResult)
    {
        Nui::val opts = Nui::val::object();
        convertOptions(opts, options);
        opts.set("forceOverwrite", options.forceOverwrite);
        const auto id = RpcClient::registerFunctionOnce([onResult = std::move(onResult)](Nui::val const& param) {
            if (param.typeOf().as<std::string>() == "null")
                onResult(std::nullopt);
            else
                onResult(std::filesystem::path{param.as<std::string>()});
            globalEventContext.executeActiveEventsImmediately();
        });
        opts.set("callbackId", id);
        RpcClient::getRemoteCallable("Nui::showSaveDialog")(opts);
    }
    // #####################################################################################################################
}