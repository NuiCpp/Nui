#pragma once

namespace Nui
{
    /**
     * @brief Utility class to detect if an object was moved.
     */
    class MoveDetector
    {
      public:
        MoveDetector() = default;
        MoveDetector(MoveDetector const&) = default;
        MoveDetector(MoveDetector&& other) noexcept
        {
            other.wasMoved_ = true;
        }
        MoveDetector& operator=(MoveDetector const&) = default;
        MoveDetector& operator=(MoveDetector&& other) noexcept
        {
            other.wasMoved_ = true;
            return *this;
        }

        bool wasMoved() const noexcept
        {
            return wasMoved_;
        }

      private:
        bool wasMoved_{false};
    };
}