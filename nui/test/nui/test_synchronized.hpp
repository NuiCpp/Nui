#pragma once

#include <gtest/gtest.h>

#include "common_test_fixture.hpp"
#include "engine/global_object.hpp"
#include "engine/document.hpp"
#include "engine/object.hpp"

#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/event_system/tags.hpp>

namespace Nui::Tests
{
    using namespace Engine;
    using namespace std::string_literals;

    class TestSynchronized : public CommonTestFixture
    {};

    TEST_F(TestSynchronized, CanSetObservedAttributeOnChild)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string, NUI_SYNCHRONIZE> observedClass{"asdf"};

        render(div{}(div{class_ = observedClass}()));

        EXPECT_FALSE(Nui::val::global("document")["body"].hasOwnProperty("attributes"));
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"].as<Object const&>().size(), 1);
    }

    TEST_F(TestSynchronized, ChangingObservedValueChangesTheDomWhenEventsAreProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string, NUI_SYNCHRONIZE> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");

        observedClass = "qwer";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "qwer");
    }

    TEST_F(TestSynchronized, ChangingObservedValuesDoesNotChangeTheDomWhenEventsAreNotProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string, NUI_SYNCHRONIZE> observedClass{"asdf"};

        render(div{class_ = observedClass}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        observedClass = "qwer";
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
    }

    TEST_F(TestSynchronized, CanSetMixedAttributes)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        Observed<std::string, NUI_SYNCHRONIZE> observedClass{"asdf"};

        render(div{class_ = observedClass, id = "qwer"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestSynchronized, ChangeInObservedValueOnlyChangesRelatedAttribute)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;
        using Nui::Attributes::id;

        Observed<std::string, NUI_SYNCHRONIZE> observedClass{"asdf"};
        Observed<std::string, NUI_SYNCHRONIZE> observedId{"qwer"};

        render(div{class_ = observedClass, id = observedId}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
        observedClass = "zxcv";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "zxcv");
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "qwer");
    }

    TEST_F(TestSynchronized, ChangeInObservedValueOnlyChangesRelatedAttributeInHierarchy)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string, NUI_SYNCHRONIZE> observedClass{"asdf"};
        Observed<std::string, NUI_SYNCHRONIZE> observedClassNested{"qwer"};

        render(div{class_ = observedClass}(div{class_ = observedClassNested}()));

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "asdf");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["class"].as<std::string>(), "qwer");
        observedClass = "zxcv";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "zxcv");
    }

    TEST_F(TestSynchronized, AttributeGeneratorUpdatesDomOnObservedValueChange)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<int, NUI_SYNCHRONIZE> observedClass{0};

        render(div{class_ = observe(observedClass).generate([&observedClass]() {
            return "C"s + std::to_string(observedClass.value());
        })}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "C0");
        observedClass = 1;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "C1");
    }

    TEST_F(TestSynchronized, GeneratorCanUpdateOnMultipleObservedValues)
    {
        using Nui::Elements::div;
        using namespace Nui::Attributes;

        Nui::val ref;
        Observed<int, NUI_SYNCHRONIZE> number{0};
        Observed<std::string, NUI_SYNCHRONIZE> text{"asdf"};

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

    TEST_F(TestSynchronized, StyleAttributeCanBeString)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        render(div{style = "color: red"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
    }

    TEST_F(TestSynchronized, StyleAttributeCanBeObservedString)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        Observed<std::string, NUI_SYNCHRONIZE> observedStyle{"color: red"};
        render(div{style = observedStyle}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
    }

    TEST_F(TestSynchronized, StyleAttributeCanBeObservedStringAndChangesWhenObservedValueChanges)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        Observed<std::string, NUI_SYNCHRONIZE> observedStyle{"color: red"};
        render(div{style = observedStyle}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
        observedStyle = "color: blue";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: blue");
    }

    TEST_F(TestSynchronized, StyleAttributeCanUseGenerator)
    {
        using Nui::Elements::div;
        using Nui::Attributes::style;

        Observed<std::string, NUI_SYNCHRONIZE> divColor{"red"};
        render(div{style = observe(divColor).generate([&divColor]() {
            return "color: "s + divColor.value();
        })}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: red");
        divColor = "blue";
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["style"].as<std::string>(), "color: blue");
    }

    TEST_F(TestSynchronized, EventIsCallable)
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

    TEST_F(TestSynchronized, EventIsCallableWithEvent)
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

    TEST_F(TestSynchronized, EventAutomaticallyUpdatesObservedChanges)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;
        using Nui::Attributes::onClick;

        Observed<bool, NUI_SYNCHRONIZE> clicked{false};
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

    TEST_F(TestSynchronized, CanGetReferenceToElement)
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

    TEST_F(TestSynchronized, CanGetReferenceToElementUsingFunction)
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

    TEST_F(TestSynchronized, CanGetReferenceToElementWithChildren)
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

    TEST_F(TestSynchronized, ModificationProxyCausesUpdates)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Observed<std::string, NUI_SYNCHRONIZE> idValue{"A"};

        render(div{id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
        {
            auto proxy = idValue.modify();
            *proxy = "B";
        }
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestSynchronized, AttributeCanBeVariant1)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        std::variant<std::string, int> idValue{"A"};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestSynchronized, AttributeCanBeVariant2)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        std::variant<std::string, int> idValue{2};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<long long>(), 2);
    }

    TEST_F(TestSynchronized, AttributeCanBeObservedVariant)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Observed<std::variant<std::string, int>, NUI_SYNCHRONIZE> idValue{"A"};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        idValue = 2;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<long long>(), 2);
    }

    TEST_F(TestSynchronized, CanUseAttributeLiteral)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;
        using namespace Nui::Attributes::Literals;

        render(div{"id"_attr = "A"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestSynchronized, OptionalIsUnsetByObserved)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Observed<std::optional<std::string>, NUI_SYNCHRONIZE> idValue{std::string{"A"}};

        render(div{id = idValue}());
        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        idValue = std::nullopt;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_FALSE(Nui::val::global("document")["body"]["attributes"].hasOwnProperty("id"));
    }

    TEST_F(TestSynchronized, CanSetDeferredAttribute)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        render(div{!id = "hi"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "hi");
    }

    TEST_F(TestSynchronized, CanSetDeferredAttribute2)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        render(div{!(id) = "hi"}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "hi");
    }

    TEST_F(TestSynchronized, CanUseSharedPointerObserved)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");

        render(div{id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestSynchronized, CanUseWeakPointerObserved)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");
        auto weakIdValue = std::weak_ptr{idValue};

        render(div{id = weakIdValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestSynchronized, CanUseSharedPointerObservedWithDeferred)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");

        render(div{!id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestSynchronized, CanUseWeakPointerObservedWithDeferred)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");
        auto weakIdValue = std::weak_ptr{idValue};

        render(div{!id = weakIdValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        *idValue = "B";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "B");
    }

    TEST_F(TestSynchronized, WeakPointerAttributeDoesNotFailOnExpired)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");
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

    TEST_F(TestSynchronized, SharedPointerAttributeDoesNotFailOnExpired)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");

        render(div{id = idValue}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");

        {
            *idValue = "B";
        }
        idValue = nullptr;
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["id"].as<std::string>(), "A");
    }

    TEST_F(TestSynchronized, SharedPointerMayExpireBeforeDetach)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Nui::Observed<bool, NUI_SYNCHRONIZE> other{true};
        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");

        render(div{}(observe(other), [&idValue]() -> Nui::ElementRenderer {
            return div{id = idValue}();
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "A");

        other = false;
        EXPECT_NO_FATAL_FAILURE(globalEventContext.executeActiveEventsImmediately());
    }

    TEST_F(TestSynchronized, WeakPointerMayExpireBeforeDetach)
    {
        using Nui::Elements::div;
        using Nui::Attributes::id;

        Nui::Observed<bool, NUI_SYNCHRONIZE> other{true};
        auto idValue = std::make_shared<Observed<std::string, NUI_SYNCHRONIZE>>("A");
        auto weakIdValue = std::weak_ptr{idValue};

        render(div{}(observe(other), [&weakIdValue]() -> Nui::ElementRenderer {
            return div{id = weakIdValue}();
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["attributes"]["id"].as<std::string>(), "A");

        other = false;
        EXPECT_NO_FATAL_FAILURE(globalEventContext.executeActiveEventsImmediately());
    }

    TEST_F(TestSynchronized, CanUseLvalueLambdaForGenerate)
    {
        using Nui::Elements::body;
        using Nui::Attributes::class_;

        auto lambda = [this]() -> std::string {
            (void)this;
            return "Hello";
        };
        Observed<bool, NUI_SYNCHRONIZE> bla{true};

        render(body{class_ = observe(bla).generate(lambda)}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "Hello");
    }

    TEST_F(TestSynchronized, GeneratorCanTakeObservedValuesAsArguments)
    {
        using Nui::Elements::div;
        using Nui::Attributes::class_;

        Observed<std::string, NUI_SYNCHRONIZE> classPart1{"Hello"};
        Observed<std::string, NUI_SYNCHRONIZE> classPart2{"World"};

        render(
            div{class_ = observe(classPart1, classPart2)
                             .generate([](std::string const& part1, std::string const& part2) -> std::string {
                                 return part1 + " " + part2;
                             })}());

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "Hello World");

        classPart1 = "Goodbye";
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["attributes"]["class"].as<std::string>(), "Goodbye World");
    }

    TEST_F(TestSynchronized, SubscriptOperatorAssignmentUpdatesView)
    {
        Nui::val parent;
        Observed<std::vector<char>, NUI_SYNCHRONIZE> vec{{'A', 'B', 'C', 'D'}};

        rangeTextBodyRender(vec, parent);
        textBodyParityTest(vec, parent);

        vec[2] = 'X';
        globalEventContext.executeActiveEventsImmediately();
        textBodyParityTest(vec, parent);
    }

    TEST_F(TestSynchronized, ModificationIsPerformedAtRightLocationWithNilPrefix)
    {
        Nui::val parent;

        Observed<std::vector<char>, NUI_SYNCHRONIZE> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        // clang-format off
        render(
            body{reference = parent}(
                range(vec).before(
                    Nui::nil(),
                    div{}("Prefix2")
                ),
                [&vec](long long, auto const& element) {
                    return div{}(std::string{element});
                }
            )
        );
        // clang-format on

        vec[1] = 'X';
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(parent["children"]["length"].as<long long>(), static_cast<long long>(vec.size() + 1));
        EXPECT_EQ(parent["children"][0]["textContent"].as<std::string>(), "Prefix2");
        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(parent["children"][i + 1]["textContent"].as<std::string>(), std::string{vec[i]});
        }
    }

    TEST_F(TestSynchronized, PropertiesDoNotChangeIfEventsAreNotProcessed)
    {
        using Nui::Elements::div;
        using Nui::Attributes::checked;
        using namespace Nui::Attributes::Literals;

        Observed<bool, NUI_SYNCHRONIZE> isChecked{false};

        render(div{"checked"_prop = isChecked}());

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<bool>(), false);

        isChecked = true;

        EXPECT_EQ(Nui::val::global("document")["body"]["checked"].as<bool>(), false);
    }

    TEST_F(TestSynchronized, CanRenderUsingFunctionDependingOnObserved)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Nui::val nested;
        Observed<bool, NUI_SYNCHRONIZE> toggle{true};

        render(body{}(observe(toggle), [&toggle, &nested]() {
            if (*toggle)
            {
                return div{reference = nested}("Hello");
            }
            else
            {
                return div{reference = nested}("Goodbye");
            }
        }));

        EXPECT_EQ(Nui::val::global("document")["body"]["children"]["length"].as<long long>(), 1);
        EXPECT_EQ(nested["textContent"].as<std::string>(), "Hello");
        toggle = false;
        globalEventContext.executeActiveEventsImmediately();
        EXPECT_EQ(nested["textContent"].as<std::string>(), "Goodbye");
    }

    TEST_F(TestSynchronized, InsertOnlyDoesNecessaryUpdates)
    {
        Nui::val parent;
        Observed<std::vector<char>, NUI_SYNCHRONIZE> vec{{'A', 'B', 'C', 'D'}};

        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        std::function<Nui::ElementRenderer(char element)> renderer{[](char element) -> Nui::ElementRenderer {
            return Nui::Elements::div{}(std::string{element});
        }};

        render(body{reference = parent}(range(vec), [&vec, &renderer](long long i, auto const& element) {
            return renderer(element);
        }));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"]["length"].as<long long>(),
            static_cast<long long>(vec.size()));

        std::vector<char> renderedElements{};
        renderer = [&](char element) -> Nui::ElementRenderer {
            renderedElements.push_back(element);
            return Nui::Elements::div{}(std::string{element});
        };

        vec.insert(vec.begin() + 2, 'X');
        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"]["length"].as<long long>(),
            static_cast<long long>(vec.size()));
        EXPECT_EQ(renderedElements.size(), 1);
        EXPECT_EQ(renderedElements[0], 'X');

        for (int i = 0; i != vec.size(); ++i)
        {
            EXPECT_EQ(
                Nui::val::global("document")["body"]["children"][i]["textContent"].as<std::string>(),
                std::string{vec[i]});
        }
    }

    TEST_F(TestSynchronized, IteratorPointerReferenceWrapperBuildTest)
    {
        using Nui::Elements::div;
        using Nui::Elements::body;
        using namespace Nui::Attributes;

        Observed<std::vector<std::string>, NUI_SYNCHRONIZE> vec{{"A", "B", "C", "D"}};

        render(body{}(range(vec), [&vec](long long i, auto const& element) {
            return div{}(std::string{element});
        }));

        EXPECT_EQ(
            Nui::val::global("document")["body"]["children"]["length"].as<long long>(),
            static_cast<long long>(vec.size()));

        vec.begin()->push_back('X');
        vec[1]->push_back('Y');

        globalEventContext.executeActiveEventsImmediately();

        EXPECT_EQ(Nui::val::global("document")["body"]["children"][0]["textContent"].as<std::string>(), "AX");
        EXPECT_EQ(Nui::val::global("document")["body"]["children"][1]["textContent"].as<std::string>(), "BY");
    }
}