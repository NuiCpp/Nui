#include <nui/frontend/attributes/impl/attribute_factory.hpp>

#define MAKE_SVG_VALUE_ATTRIBUTE_RENAME(NAME, SVG_NAME) \
    namespace Nui::Attributes::Svg \
    { \
        static constexpr auto NAME = AttributeFactory{SVG_NAME}; \
    }

#define MAKE_SVG_VALUE_ATTRIBUTE(NAME) MAKE_SVG_VALUE_ATTRIBUTE_RENAME(NAME, #NAME)

#define MAKE_SVG_EVENT_ATTRIBUTE_RENAME(NAME, SVG_ACTUAL) \
    namespace Nui::Attributes::Svg \
    { \
        namespace Names \
        { \
            static constexpr auto Attr##NAME = fixToLower(SVG_ACTUAL); \
        } \
        static constexpr auto NAME = AttributeFactory{Names::Attr##NAME}; \
    }

#define MAKE_SVG_EVENT_ATTRIBUTE(NAME) MAKE_SVG_EVENT_ATTRIBUTE_RENAME(NAME, #NAME)