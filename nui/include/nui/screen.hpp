#pragma once

#include <nui/core.hpp>

#include <cstdint>
#include <vector>
#include <string>
#include <functional>

namespace Nui
{
    class Display
    {
      public:
        Display(const Display&) = default;
        Display(Display&&) = default;
        Display& operator=(const Display&) = default;
        Display& operator=(Display&&) = default;
        ~Display() = default;

        int32_t x() const
        {
            return x_;
        }
        int32_t y() const
        {
            return y_;
        }
        int32_t width() const
        {
            return width_;
        }
        int32_t height() const
        {
            return height_;
        }
        bool isPrimary() const
        {
            return isPrimary_;
        }
        std::string deviceName() const
        {
            return deviceName_;
        }

        Display(int32_t x, int32_t y, int32_t width, int32_t height, bool isPrimary, std::string deviceName)
            : x_(x)
            , y_(y)
            , width_(width)
            , height_(height)
            , isPrimary_(isPrimary)
            , deviceName_(std::move(deviceName))
        {}

      private:
        int32_t x_;
        int32_t y_;
        int32_t width_;
        int32_t height_;
        bool isPrimary_;
        std::string deviceName_;
    };

#ifdef NUI_BACKEND
    class Screen
    {
      public:
        static std::vector<Display> getDisplays();
        static Display getPrimaryDisplay();
    };
#else
    class Screen
    {
      public:
        static void getDisplays(std::function<void(std::vector<Display>&&)> callback);
        static void getPrimaryDisplay(std::function<void(Display&&)> callback);
    };
#endif
}