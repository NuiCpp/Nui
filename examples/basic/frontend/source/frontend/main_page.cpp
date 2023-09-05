#include <frontend/main_page.hpp>

#include <nui/frontend/elements.hpp>

Nui::ElementRenderer MainPage::render() {
  using namespace Nui;
  using namespace Nui::Elements;
  using Nui::Elements::div; // because of the global div.

  return body{}("Hello World");
}