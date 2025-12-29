#pragma once

#include <nui/frontend/val_wrapper.hpp>
#include <nui/frontend/api/file_list.hpp>
#include <nui/frontend/api/data_transfer_item_list.hpp>

#include <vector>
#include <string>

namespace Nui::WebApi
{
    /**
     * @see https://developer.mozilla.org/en-US/docs/Web/API/Event
     */
    class DataTransfer : public ValWrapper
    {
      public:
        DataTransfer();
        explicit DataTransfer(Nui::val dataTransfer);

        enum class DropEffect
        {
            None,
            Copy,
            Move,
            Link
        };
        /**
         * @brief Gets the type of drag-and-drop operation currently selected or sets the operation to a new type.
         */
        DropEffect dropEffect() const;

        enum class EffectAllowed
        {
            None,
            Copy,
            CopyLink,
            CopyMove,
            Link,
            LinkMove,
            Move,
            All,
            Uninitialized
        };
        /**
         * @brief Provides all of the types of operations that are possible.
         */
        EffectAllowed effectAllowed() const;

        /**
         * @brief Contains a list of all the local files available on the data transfer. If the drag operation doesn't
         * involve dragging files, this property is an empty list.
         */
        FileList files() const;

        /**
         * @brief Gives a DataTransferItemList object which is a list of all of the drag data.
         */
        DataTransferItemList items() const;

        /**
         * @brief The DataTransfer.types read-only property returns the available types that exist in the items.
         */
        std::vector<std::string> types() const;

        /**
         * @brief Remove the data associated with all types.
         */
        void clearData();

        /**
         * @brief Remove the data associated with a given type.
         */
        void clearData(std::string const& format);

        /**
         * @brief The DataTransfer.getData() method retrieves drag data (as a string) for the specified type. If the
         * drag operation does not include data, this method returns an empty string.
         */
        std::string getData(std::string const& format) const;

        /**
         * @brief The DataTransfer.setData() method sets the drag operation's drag data to the specified data and type.
         * If data for the given type does not exist, it is added at the end of the drag data store, such that the last
         * item in the types list will be the new type. If data for the given type already exists, the existing data is
         * replaced in the same position. That is, the order of the types list is not changed when replacing data of the
         * same type. Example data types are text/plain and text/uri-list.
         */
        void setData(std::string const& format, std::string const& data) const;

        /**
         * @brief When a drag occurs, a translucent image is generated from the drag target (the element the dragstart
         * event is fired at), and follows the mouse pointer during the drag. This image is created automatically, so
         * you do not need to create it yourself. However, if a custom image is desired, the DataTransfer.setDragImage()
         * method can be used to set the custom image to be used. The image will typically be an <img> element but it
         * can also be a <canvas> or any other visible element.
         */
        void setDragImage(Nui::val imgElement, long xOffset, long yOffset) const;
    };
}