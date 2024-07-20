#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"
#include "engine/object.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

namespace Nui::Tests
{
    using namespace Engine;
    using namespace std::string_literals;

    class TestAttributes : public CommonTestFixture
    {};

    TEST_F(TestAttributes, AttributeIsSetOnElement)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        render(div{class_ = "asdf"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, MultipleAttributesAreSet)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        render(div{class_ = "asdf", id = "qwer"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"].as<Object const&>().size(), 2);
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, AttributeIsSetOnElementWithChildren)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        render(div{class_ = "asdf"}(div{}()));

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"].as<Object const&>().size(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, AttributeIsSetOnChild)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        render(div{}(div{class_ = "asdf"}()));

        EXPECT_FALSE(Nui::val::global("document")["body"].hasOwnProperty("attributes"));
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"].as<Object const&>().size(), 1);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, MultipleAttributesAreSetOnChild)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        render(div{}(div{class_ = "asdf", id = "qwer"}()));

        EXPECT_FALSE(Nui::val::global("document")["body"].hasOwnProperty("attributes"));
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"].as<Object const&>().size(), 2);
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, CanSetObservedAttribute)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, CanSetObservedAttributeOnChild)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{}(div{class_ = observedClass}()));

        EXPECT_FALSE(Nui::val::global("document")["body"].hasOwnProperty("attributes"));
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"].as<Object const&>().size(), 1);
    }

    TEST_F(TestAttributes, ChangingObservedValueChangesTheDomWhenEventsAreProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");

        observedClass = "qwer";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, ChangingObservedValuesDoesNotChangeTheDomWhenEventsAreNotProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        observedClass = "qwer";
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestAttributes, CanSetMixedAttributes)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        Observed<std::string> observedClass{"asdf"};

        render(div{class_ = observedClass, id = "qwer"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, ChangeInObservedValueOnlyChangesRelatedAttribute)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        Observed<std::string> observedClass{"asdf"};
        Observed<std::string> observedId{"qwer"};

        render(div{class_ = observedClass, id = observedId}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
        observedClass = "zxcv";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "zxcv");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestAttributes, ChangeInObservedValueOnlyChangesRelatedAttributeInHierarchy)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string> observedClass{"asdf"};
        Observed<std::string> observedClassNested{"qwer"};

        render(div{class_ = observedClass}(div{class_ = observedClassNested}()));

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["class"].as<std::string>(), "qwer");
        observedClass = "zxcv";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "zxcv");
    }

    TEST_F(TestAttributes, AttributeGeneratorUpdatesDomOnObservedValueChange)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<int> observedClass{0};

        render(div{class_ = observe(observedClass).generate([&observedClass]() {
            return "C"s + std::to_string(observedClass.value());
        })}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "C0");
        observedClass = 1;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "C1");
    }

    TEST_F(TestAttributes, GeneratorCanUpdateOnMultipleObservedValues)
    {
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        Nui::val ref;
        Observed<int> number{0};
        Observed<std::string> text{"asdf"};

        render(div{reference = ref, class_ = observe(number, text).generate([&number, &text]() {
                       return text.value() + std::to_string(number.value());
                   })}());

        EXPECT_EQ(ref["attributes"]["class"].as<std::string>(), "asdf0");

        number = 1;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(ref["attributes"]["class"].as<std::string>(), "asdf1");

        text = "qwer";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(ref["attributes"]["class"].as<std::string>(), "qwer1");
    }

    TEST_F(TestAttributes, StyleAttributeCanBeString)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        render(div{style = "color: red"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
    }

    TEST_F(TestAttributes, StyleAttributeCanBeObservedString)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        Observed<std::string> observedStyle{"color: red"};
        render(div{style = observedStyle}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
    }

    TEST_F(TestAttributes, StyleAttributeCanBeObservedStringAndChangesWhenObservedValueChanges)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        Observed<std::string> observedStyle{"color: red"};
        render(div{style = observedStyle}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
        observedStyle = "color: blue";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: blue");
    }

    TEST_F(TestAttributes, StyleAttributeCanUseGenerator)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        Observed<std::string> divColor{"red"};
        render(div{style = observe(divColor).generate([&divColor]() {
            return "color: "s + divColor.value();
        })}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
        divColor = "blue";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: blue");
    }

    TEST_F(TestAttributes, StyleAttributeCanUseUtilityClassWithStaticValues)
    {
        using Nui::Elements::div;
        using Nui::Attributes::Style;
        using Nui::Attributes::style;
        using namespace Nui::Attributes::Literals;

        render(
            div{style = Style{
                    "color"_style = "red",
                    "background-color"_style = "blue",
                }}());

        EXPECT_EQ(
            Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(),
            "color:red;background-color:blue");
    }

    TEST_F(TestAttributes, StyleAttributeCanUseUtilityClassWithObservedValues)
    {
        using Nui::Elements::div;
        using Nui::Attributes::Style;
        using Nui::Attributes::style;
        using namespace Nui::Attributes::Literals;

        Observed<std::string> color{"red"};
        Observed<std::string> backgroundColor{"blue"};
        render(
            div{style = Style{
                    "color"_style = color,
                    "background-color"_style = backgroundColor,
                }}());

        EXPECT_EQ(
            Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(),
            "color:red;background-color:blue");
        color = "green";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(
            Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(),
            "color:green;background-color:blue");
        backgroundColor = "yellow";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(
            Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(),
            "color:green;background-color:yellow");
    }

    TEST_F(TestAttributes, StyleAttributeCanUseUtilityClassWithObservedValuesAndGenerator)
    {
        using Nui::Elements::div;
        using Nui::Attributes::Style;
        using Nui::Attributes::style;
        using namespace Nui::Attributes::Literals;

        Observed<std::string> color{"red"};
        Observed<std::string> backgroundColor{"blue"};
        render(
            div{style = Style{
                    "color"_style = observe(color).generate([&color]() {
                        return color.value();
                    }),
                    "background-color"_style = observe(backgroundColor).generate([&backgroundColor]() {
                        return backgroundColor.value();
                    }),
                }}());

        EXPECT_EQ(
            Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(),
            "color:red;background-color:blue");
        color = "green";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(
            Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(),
            "color:green;background-color:blue");
        backgroundColor = "yellow";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(
            Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(),
            "color:green;background-color:yellow");
    }

    TEST_F(TestAttributes, EventIsCallable)
    {
        using Nui::Elements::div;
        using Nui::Attributes::onClick;

        bool clicked = false;
        render(div{onClick = [&clicked]() {
            clicked = true;
        }}());

        Nui::val::global("document")["body"]["onclick"](Nui::val{});
        EXPECT_TRUE(clicked);
    }

    TEST_F(TestAttributes, EventIsCallableWithEvent)
    {
        using Nui::Elements::div;
        using Nui::Attributes::onClick;

        std::optional<long long> prop = 0;
        render(div{onClick = [&prop](const Nui::val& event) {
            prop = event["prop"].as<long long>();
        }}());

        Object event;
        event["prop"] = 2;
        Nui::val::global("document")["body"]["onclick"](Nui::val{createValue(event)});
        EXPECT_EQ(prop, 2);
    }

    TEST_F(TestAttributes, EventAutomaticallyUpdatesObservedChanges)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;
        using Nui::Attributes::onClick;

        Observed<bool> clicked{false};
        render(
            div{onClick =
                    [&clicked]() {
                        clicked = true;
                    },
                id = observe(clicked).generate([&clicked]() {
                    return clicked.value() ? "clicked" : "not clicked";
                })}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "not clicked");
        Nui::val::global("document")["body"]["onclick"](Nui::val{});
        EXPECT_TRUE(clicked.value());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "clicked");
    }

    TEST_F(TestAttributes, CanGetReferenceToElement)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using Nui::Attributes::id;
        using Nui::Attributes::reference;

        std::weak_ptr<Dom::BasicElement> ref;

        render(body{id = "A", reference = ref}("Hello World"));

        ASSERT_FALSE(ref.expired());
        EXPECT_EQ(ref.lock()->val()["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestAttributes, CanGetReferenceToElementUsingFunction)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using Nui::Attributes::id;
        using Nui::Attributes::reference;

        std::weak_ptr<Dom::BasicElement> ref;

        render(body{id = "A", reference = [&](auto&& weak) {
                        ref = std::move(weak);
                    }}("Hello World"));

        ASSERT_FALSE(ref.expired());
        EXPECT_EQ(ref.lock()->val()["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestAttributes, CanGetReferenceToElementWithChildren)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using Nui::Attributes::id;
        using Nui::Attributes::reference;

        std::weak_ptr<Dom::BasicElement> ref;

        render(body{id = "A", reference = ref}(div{id = "B"}()));

        ASSERT_FALSE(ref.expired());
        EXPECT_EQ(ref.lock()->val()["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestAttributes, ModificationProxyCausesUpdates)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Observed<std::string> idValue{"A"};

        render(div{id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
        {
            auto proxy = idValue.modify();
            *proxy = "B";
        }
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestAttributes, AttributeCanBeVariant1)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        std::variant<std::string, int> idValue{"A"};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestAttributes, AttributeCanBeVariant2)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        std::variant<std::string, int> idValue{2};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<long long>(), 2);
    }

    TEST_F(TestAttributes, AttributeCanBeObservedVariant)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Observed<std::variant<std::string, int>> idValue{"A"};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        idValue = 2;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<long long>(), 2);
    }

    TEST_F(TestAttributes, CanUseAttributeLiteral)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;
        using namespace Nui::Attributes::Literals;

        render(div{"id"_attr = "A"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestAttributes, OptionalIsUnsetByObserved)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Observed<std::optional<std::string>> idValue{std::string{"A"}};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        idValue = std::nullopt;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_FALSE(Nui::val::global("document")["body"]["attributes"].hasOwnProperty("id"));
    }

    TEST_F(TestAttributes, CanSetDeferredAttribute)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        render(div{!id = "hi"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "hi");
    }

    TEST_F(TestAttributes, CanUseSharedPointerObserved)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string>>("A");

        render(div{id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestAttributes, CanUseWeakPointerObserved)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string>>("A");
        auto weakIdValue = std::weak_ptr{idValue};

        render(div{id = weakIdValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestAttributes, CanUseSharedPointerObservedWithDeferred)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string>>("A");

        render(div{!id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestAttributes, CanUseWeakPointerObservedWithDeferred)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string>>("A");
        auto weakIdValue = std::weak_ptr{idValue};

        render(div{!id = weakIdValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestAttributes, WeakPointerAttributeDoesNotFailOnExpired)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string>>("A");
        auto weakIdValue = std::weak_ptr{idValue};

        render(div{id = weakIdValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        {
            *idValue = "B";
        }
        idValue = nullptr;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestAttributes, SharedPointerAttributeDoesNotFailOnExpired)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string>>("A");

        render(div{id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        {
            *idValue = "B";
        }
        idValue = nullptr;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestAttributes, SharedPointerMayExpireBeforeDetach)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Nui::Observed<bool> other{true};
        auto idValue = std::make_shared<Observed<std::string>>("A");

        render(div{}(observe(other), [&idValue]() -> Nui::ElementRenderer {
            return div{id = idValue}();
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "A");

        other = false;
        EXPECT_NO_FATAL_FAILURE(globalEventContext.executeActiveEventsImmediately());
    }

    TEST_F(TestAttributes, WeakPointerMayExpireBeforeDetach)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Nui::Observed<bool> other{true};
        auto idValue = std::make_shared<Observed<std::string>>("A");
        auto weakIdValue = std::weak_ptr{idValue};

        render(div{}(observe(other), [&weakIdValue]() -> Nui::ElementRenderer {
            return div{id = weakIdValue}();
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "A");

        other = false;
        EXPECT_NO_FATAL_FAILURE(globalEventContext.executeActiveEventsImmediately());
    }
}