#include <nui/backend/rpc_hub.hpp>

#include <nui/backend/filesystem/file_dialog.hpp>

namespace Nui
{
    // #####################################################################################################################
    RpcHub::RpcHub(Window& window)
        : window_{&window}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableFileDialogs() const
    {
        registerFunction("nui_showOpenDialog", [this](nlohmann::json const& args) {
            const auto callbackId = args["callbackId"].get<std::string>();
            auto result = FileDialog::showOpenDialog(args.get<FileDialog::OpenDialogOptions>());
            nlohmann::json response;
            if (result)
                response = *result;
            callRemote(callbackId, response);
        });
        registerFunction("nui_showDirectoryDialog", [this](nlohmann::json const& args) {
            const auto callbackId = args["callbackId"].get<std::string>();
            const auto result = FileDialog::showDirectoryDialog(args.get<FileDialog::DirectoryDialogOptions>());
            nlohmann::json response;
            if (result)
                response = *result;
            callRemote(callbackId, response);
        });
        registerFunction("nui_showSaveDialog", [this](nlohmann::json const& args) {
            const auto callbackId = args["callbackId"].get<std::string>();
            const auto result = FileDialog::showSaveDialog(args.get<FileDialog::SaveDialogOptions>());
            nlohmann::json response;
            if (result)
                response = result->string();
            callRemote(callbackId, response);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableWindowFunctions() const
    {
        registerFunction("nui_openDevTools", [this]() {
            window_->openDevTools();
        });
        registerFunction("nui_terminate", [this]() {
            window_->terminate();
        });
        registerFunction("nui_setWindowSize", [this](int width, int height, int hint) {
            window_->setSize(width, height, static_cast<WebViewHint>(hint));
        });
        registerFunction("nui_setWindowTitle", [this](std::string const& title) {
            window_->setTitle(title);
        });
    }
    //---------------------------------------------------------------------------------------------------------------------
    void RpcHub::enableAll()
    {
        enableFileDialogs();
        enableWindowFunctions();
    }
    // #####################################################################################################################
}