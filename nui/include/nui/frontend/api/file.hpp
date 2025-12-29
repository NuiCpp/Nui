#pragma once

#include <nui/frontend/val_wrapper.hpp>

#include <chrono>
#include <string>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
     */
    class File : public ValWrapper
    {
      public:
        explicit File(Nui::val file);

        std::chrono::system_clock::time_point lastModified() const;
        std::string name() const;
        std::string webkitRelativePath() const;
    };
}