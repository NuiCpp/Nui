#pragma once

#include <include/cef_task.h>

#include <exception>
#include <functional>
#include <iostream>

namespace Nui::Detail::Cef
{
    class FunctionTask : public CefTask
    {
      public:
        explicit FunctionTask(std::function<void()> function)
            : function_{std::move(function)}
        {}

        void Execute() override
        {
            try
            {
                function_();
            }
            catch (std::exception const& exception)
            {
                std::cerr << "Nui: exception in UI task: " << exception.what() << std::endl;
            }
        }

      private:
        std::function<void()> function_;
        IMPLEMENT_REFCOUNTING(FunctionTask);
    };

    /**
     * @brief Queues the function on the CEF UI thread, also when called from it. The engine must be running.
     */
    inline void postToUi(std::function<void()> function)
    {
        CefPostTask(TID_UI, CefRefPtr<CefTask>{new FunctionTask(std::move(function))});
    }
}
