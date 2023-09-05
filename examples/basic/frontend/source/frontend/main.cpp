#include <frontend/main_page.hpp>

#include <nui/core.hpp>
#include <nui/window.hpp>

#include <nui/frontend/dom/dom.hpp>

#include <memory>

static std::unique_ptr<MainPage> mainPage{};
static std::unique_ptr<Nui::Dom::Dom> dom{};

extern "C" void frontendMain()
{
    mainPage = std::make_unique<MainPage>();
    dom = std::make_unique<Nui::Dom::Dom>();
    dom->setBody(mainPage->render());
}

EMSCRIPTEN_BINDINGS(nui_example_frontend)
{
    emscripten::function("main", &frontendMain);
}
#include <nui/frontend/bindings.hpp>