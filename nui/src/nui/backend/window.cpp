#include <nui/window.hpp>

#include <nui/backend/filesystem/special_paths.hpp>
#include <webview.h>
#include <random>

#include <fstream>
#include <filesystem>

namespace Nui
{
    //#####################################################################################################################
    struct Window::Implementation
    {
        webview::webview view;
        std::vector<std::filesystem::path> cleanupFiles;

        Implementation(bool debug)
            : view{debug}
        {}
    };
    //#####################################################################################################################
    Window::Window()
        : Window{false}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(std::string const& title, bool debug)
        : Window{debug}
    {
        setTitle(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(bool debug)
        : impl_{std::make_unique<Implementation>(debug)}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::Window(char const* title, bool debug)
        : Window{std::string{title}, debug}
    {}
    //---------------------------------------------------------------------------------------------------------------------
    Window::~Window()
    {
        for (auto const& file : impl_->cleanupFiles)
            std::filesystem::remove(file);
    }
    Window::Window(Window&&) = default;
    Window& Window::operator=(Window&&) = default;
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setTitle(std::string const& title)
    {
        impl_->view.set_title(title);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setSize(int width, int height, WebViewHint hint)
    {
        impl_->view.set_size(width, height, static_cast<int>(hint));
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::setHtml(std::string_view html)
    {
#if defined(_WIN32)
        // https://github.com/MicrosoftEdge/WebView2Feedback/issues/1355
        // :((((

        using namespace std::string_literals;
        constexpr static auto fileNameSize = 25;
        std::string_view alphanum =
            "0123456789"
            "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
            "abcdefghijklmnopqrstuvwxyz";
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<std::size_t> dis(0, alphanum.size() - 1);
        std::string fileName(fileNameSize, '\0');
        for (std::size_t i = 0; i < fileNameSize; ++i)
            fileName[i] = alphanum[dis(gen)];
        const auto tempFile = resolvePath("%temp%/"s + fileName + ".html");
        {
            std::ofstream temporary{tempFile, std::ios_base::binary};
            temporary.write(html.data(), static_cast<std::streamsize>(html.size()));
        }
        impl_->view.navigate("file://"s + tempFile.string());
        impl_->cleanupFiles.push_back(tempFile);
#else
        impl_->view.set_html(std::string{html});
#endif
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::navigate(const std::string& url)
    {
        impl_->view.navigate(url);
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::run()
    {
        impl_->view.run();
    }
    //---------------------------------------------------------------------------------------------------------------------
    void Window::terminate()
    {
        impl_->view.terminate();
    }
    //#####################################################################################################################
}