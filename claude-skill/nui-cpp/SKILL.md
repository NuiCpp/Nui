---
name: nui-cpp
description: >
  Expert guidance for writing frontend code using NuiCpp -- a C++ WebAssembly (WASM) frontend
  library compiled via Emscripten. Conceptually similar to JSX but expressed entirely in C++.
  Use this skill whenever the user asks to build UI components, pages, or applications using
  NuiCpp, Nui, or "C++ frontend/WASM UI". Also trigger when the user mentions Nui::Observed,
  Nui::range, ElementRenderer, Nui::Elements, Nui::Attributes, StateTransformer, ChangePolicy,
  ValWrapper, convertToVal, convertFromVal, or any other Nui:: namespace identifiers. Trigger
  even for partial tasks like "add a button to my Nui component", "how do I handle events in
  Nui", or "write a function component". This skill covers element syntax, reactive state,
  range rendering, class components (functor + pimpl), function components, StateTransformer,
  nil/fragment, WebAPI event wrappers, val conversion, JS interop via emscripten::val, and
  reactive side-effects via listen/smartListen/ListenRemover.
---

# NuiCpp Frontend Skill

NuiCpp is a C++ WASM frontend library (Emscripten). Think of it as JSX-in-C++: you build
DOM trees using a C++ DSEL (domain-specific embedded language). Always apply the patterns
below precisely -- the syntax is strict and deviations cause compile errors.

---

## Element Syntax

```cpp
div{
    class_ = "my-class",
    id = "my-id",
    onClick = [](Nui::val event) { std::cout << "clicked\n"; },
    "change"_event = [](Nui::val event) { /* custom event */ },
    "data-foo"_attr = "bar",          // custom data attributes
    "value"_prop = "something",       // DOM properties
}(
    /* children - parentheses are NEVER omitted */
    span{}("child text"),
    text{"plain text content"}()      // use text{} for mixed text+children
)
```

**Key rules:**
- Children go inside `()` after the `{}` attribute block. Always include `()` even if empty.
- Mix text with other children using `text{"..."}()`, not raw string literals alongside elements.
- Supported named event wrappers: `Nui::WebApi::MouseEvent`, `Nui::WebApi::Event`,
  `Nui::WebApi::KeyboardEvent`, `Nui::WebApi::DragEvent`.

---

## Reactive State -- `Nui::Observed`

```cpp
#include <nui/event_system/observed_value.hpp>

Nui::Observed<std::string> label{"Hello"};
Nui::Observed<std::vector<std::string>> items;
```

### Mutation semantics

| Intent | Syntax | Effect |
|---|---|---|
| Mutate + track change | `items.push_back("x")` | Queues UI update |
| Mutate silently | `items.value().push_back("x")` | No UI update |
| Full invalidation | `items.modify()` | Full range redraw |

Use `.modify()` when changing 50% or more of a vector's elements.

### Flushing updates

Changes are **queued**, not applied immediately.
- **Inside event handlers** -- flushed automatically at handler end.
- **Outside event handlers** (timers, async, mousemove) -- call manually:
  ```cpp
  Nui::globalEventContext.executeActiveEventsImmediately();
  ```

Do **not** copy `Nui::Observed` into temporaries when mutating -- always mutate the original.

---

## Listening to Observed Changes -- `listen` / `smartListen`

```cpp
#include <nui/event_system/listen.hpp>
```

Use `listen` / `smartListen` to run **imperative side-effects** whenever an `Observed` value
changes (logging, fetching, cascading state updates, etc.). This is separate from the
declarative UI bindings in the section below.

### `listen` -- raw event registration

Returns an `EventRegistry::EventIdType`. You are responsible for removing the event manually.

```cpp
// Basic: callback receives the new value by const-ref
Nui::listen(myObserved, [](std::string const& newValue) {
    Nui::WebApi::Console::log("changed to: " + newValue);
});

// Early-out: return false to automatically deregister after first fire
Nui::listen(myObserved, [](int val) -> bool {
    if (val == 42) return false;  // unregistered after this call
    return true;                  // keep listening
});

// shared_ptr overload: event is auto-removed when the Observed is destroyed
auto sharedObs = std::make_shared<Nui::Observed<int>>(0);
Nui::listen(sharedObs, [](int val) { /* ... */ });
```

**Warning:** Do **not** mutate any `Observed` from inside a `listen` callback -- this causes
infinite recursion. Use `smartListen` instead.

---

### `smartListen` -- RAII + safe side-effects

Returns a `ListenRemover` that **automatically deregisters** the listener on destruction.
The callback is delayed until after all currently-active events have finished, so it is safe
to mutate other `Observed` values inside the callback.

```cpp
// Must store the remover -- discarding it immediately removes the listener!
auto remover = Nui::smartListen(myObserved, [&otherObs](std::string const& newValue) {
    // Safe to modify other Observed values here:
    otherObs = "derived: " + newValue;
    Nui::globalEventContext.executeActiveEventsImmediately();  // flush if outside event handler
});
```

`ListenRemover` is **move-only** (no copy). Store it as a member in the component that owns
the side-effect:

```cpp
struct MyWidget::Implementation {
    Nui::Observed<std::string> query{};
    Nui::Observed<std::vector<std::string>> results{};
    Nui::ListenRemover<Nui::Observed<std::string>> queryListener;

    Implementation()
        : queryListener{Nui::smartListen(query, [this](std::string const& q) {
              // Kick off a fetch, update results, etc.
              results.assign({});
              Nui::globalEventContext.executeActiveEventsImmediately();
          })}
    {}
};
```

### `smartListen` with `shared_ptr<Observed>`

When the observed value is heap-allocated and may be destroyed independently:

```cpp
auto sharedObs = std::make_shared<Nui::Observed<int>>(0);
auto remover = Nui::smartListen(sharedObs, [](int val) {
    // callback fires only while sharedObs is alive
});
```

### Manual cleanup with `ListenRemover`

```cpp
remover.removeEvent();   // detach now (also called by destructor)
remover.disarm();        // prevent destructor from removing (ownership transfer)
```

### Quick reference

| API | Returns | Safe to mutate Observed? | Lifetime managed by |
|---|---|---|---|
| `Nui::listen(obs, fn)` | `EventIdType` | ❌ causes recursion | caller |
| `Nui::smartListen(obs, fn)` | `ListenRemover` (nodiscard) | ✅ delayed execution | `ListenRemover` RAII |

---

## Using Observed in the UI

### Reactive attribute
```cpp
div{
    class_ = Nui::observe(label).generate([](std::string const& txt) {
        return fmt::format("active {}", txt);
    })
}()
```

### Reactive style
```cpp
div{
    style = Nui::observe(visible).generate([](bool v) {
        return fmt::format("display: {};", v ? "flex" : "none");
    })
}()
```

### Reactive text content
```cpp
div{}(label)   // renders label's value as text, auto-updates
```

### Reactive subtree (`Nui::observe` + renderer)
Must be the **only logical child** of its parent:
```cpp
div{}(
    Nui::observe(label),
    [](std::string const& txt) -> Nui::ElementRenderer {
        return span{}(txt);
    }
)
```

---

## Range Rendering

`Nui::range` must be the **first (and only) logical child** of its parent element,
paired immediately with its renderer lambda.

### Simple range
```cpp
div{}(
    Nui::range(items),
    [](long long /*index - must be long long, not auto*/, auto const& item) -> Nui::ElementRenderer {
        return span{}(item);
    }
)
```

### Range with before/after siblings
Use `.before()` / `.after()` instead of placing elements outside the range:
```cpp
div{}(
    Nui::range(items)
        .before(
            div{}("Header"),
            div{}("Subheader")
        )
        .after(
            div{}("Footer")
        ),
    [](long long i, auto const& item) -> Nui::ElementRenderer {
        return div{}(std::to_string(item));
    }
)
```

---

## nil -- Rendering Nothing

`Nui::nil()` produces an `ElementRenderer` that inserts nothing into the DOM. Use it to
conditionally render nothing in places that syntactically require a child expression.

```cpp
#include <nui/frontend/elements/nil.hpp>

div{}(
    Nui::observe(showBanner),
    [](bool show) -> Nui::ElementRenderer {
        if (!show)
            return Nui::nil();
        return div{class_ = "banner"}("Hello!");
    }
)
```

---

## fragment -- Wrapper-free Multi-child Rendering

`Nui::Elements::fragment(...)` renders multiple children into a parent without adding an
enclosing DOM node, similar to React fragments.

```cpp
#include <nui/frontend/elements/fragment.hpp>

div{}(
    fragment(
        span{}("First"),
        span{}("Second"),
        span{}("Third")
    )
)
```

**Important limitation:** Do **not** use fragments with observed/reactive logic. Because
fragments are removed and reinserted as a unit on change, and the rerender is deferred,
their contents end up appended to the back of the parent rather than staying in place.
Keep fragments to simple, static structural groupings only.

---

## WebAPI Event Wrappers

All typed event classes (MouseEvent, KeyboardEvent, etc.) extend `Nui::ValWrapper`, which
is a lightweight, non-owning view over a `Nui::val`. Construction is cheap -- no data is
copied; all property accessors read directly from the underlying JS object.

```cpp
#include <nui/frontend/val_wrapper.hpp>   // Nui::ValWrapper base
#include <nui/frontend/api/mouse_event.hpp>
```

### Using MouseEvent
```cpp
div{
    onClick = [](Nui::WebApi::MouseEvent event) {
        double x = event.clientX();
        double y = event.clientY();
        bool shifted = event.shiftKey();
        bool ctrl = event.ctrlKey();
        int btn = event.button();   // 0=left, 1=middle, 2=right
        // event.val() gives the raw Nui::val back if needed
    }
}()
```

### Available MouseEvent properties
`clientX/Y`, `x/y`, `screenX/Y`, `pageX/Y`, `offsetX/Y`, `movementX/Y`,
`button`, `buttons`, `altKey`, `ctrlKey`, `shiftKey`, `metaKey`, `relatedTarget`.

### Other event types
| Header | Type | Typical use |
|---|---|---|
| `nui/frontend/api/event.hpp` | `Nui::WebApi::Event` | `onChange`, `onInput`, generic |
| `nui/frontend/api/keyboard_event.hpp` | `Nui::WebApi::KeyboardEvent` | `onKeyDown/Up/Press` |
| `nui/frontend/api/drag_event.hpp` | `Nui::WebApi::DragEvent` | `onDrag`, `onDrop`, etc. |
| `nui/frontend/api/ui_event.hpp` | `Nui::WebApi::UiEvent` | base of mouse/keyboard events |

All wrappers have a `.val()` accessor returning the raw `Nui::val` for any property not
directly exposed.

Using `Nui::val` directly is always valid too, for cases where the typed wrapper isn't
available or you need a property not yet wrapped:
```cpp
onClick = [](Nui::val event) {
    auto target = event["target"];
    std::string value = target["value"].as<std::string>();
}
```

---

## `convertToVal` / `convertFromVal` -- C++ to JS Conversion

```cpp
#include <nui/frontend/utility/val_conversion.hpp>
```

These are rarely needed directly -- prefer `Nui::val` member access for JS interop -- but
they are useful when passing structured C++ data to a JS function or when receiving
structured data back.

### `convertToVal` -- C++ to JS

Handles: fundamental types, `std::string`, `std::string_view`, `std::filesystem::path`,
`std::vector`, `std::map/unordered_map<std::string, T>`, `std::pair`, `std::optional`,
`std::variant`, `std::unique_ptr`, `std::shared_ptr`, `Nui::Observed<T>`, `Nui::val`,
and any boost-described struct/class.

```cpp
// Fundamental and string types
Nui::val n = Nui::convertToVal(42);
Nui::val s = Nui::convertToVal(std::string{"hello"});

// Vector becomes a JS array
std::vector<int> nums = {1, 2, 3};
Nui::val arr = Nui::convertToVal(nums);

// Map becomes a JS object
std::map<std::string, int> m = {{"x", 1}, {"y", 2}};
Nui::val obj = Nui::convertToVal(m);

// Boost-described struct becomes a JS object with matching property names
BOOST_DESCRIBE_STRUCT(MyPoint, (), (x, y))
struct MyPoint { double x; double y; };
Nui::val pt = Nui::convertToVal(MyPoint{1.0, 2.0}); // {x: 1.0, y: 2.0}

// Pass to JS function
Nui::val::global("someJsFunction").call<void>("call",
    Nui::val::global("window"), Nui::convertToVal(pt));

// std::monostate converts to JS undefined
Nui::val undef = Nui::convertToVal(std::monostate{});
```

### `convertFromVal` -- JS to C++

Mirror of the above; writes into an existing object by reference.

```cpp
Nui::val jsObj = /* ... from event or fetch response ... */;

std::string str;
Nui::convertFromVal(jsObj["name"], str);

std::vector<int> vec;
Nui::convertFromVal(jsObj["items"], vec);

// Boost-described struct: missing required members throw std::invalid_argument,
// optional members become std::nullopt if absent.
MyPoint pt;
Nui::convertFromVal(jsObj, pt);
```

### Custom ADL hooks

For types you own but cannot boost-describe, provide free functions found by ADL:
```cpp
void to_val(Nui::val& out, MyType const& t) {
    out = Nui::val::object();
    out.set("field", t.field);
}
void from_val(Nui::val const& v, MyType& t) {
    t.field = v["field"].as<decltype(t.field)>();
}
```

---

## Component Patterns

There are two component patterns. Choose based on whether the component owns private state.

| | **Class component** | **Function component** |
|---|---|---|
| Owns private `Nui::Observed` state | Yes | No |
| Keeps sub-components alive across renders | Yes | No |
| All state comes from caller | Overkill | Yes |
| Small, mostly structural/presentational | Overkill | Yes |

---

### Class Component (Functor + Pimpl)

Use when the component has its own private reactive state or owns long-lived child components.
Split into header and source; hide all heavy includes in the source.

**Header (`MyWidget.hpp`)**
```cpp
#pragma once
#include <nui/frontend/element_renderer.hpp>
#include <memory>

class MyWidget {
public:
    MyWidget();
    ~MyWidget();
    MyWidget(MyWidget const&) = delete;
    MyWidget(MyWidget&&);
    MyWidget& operator=(MyWidget const&) = delete;
    MyWidget& operator=(MyWidget&&);

    Nui::ElementRenderer operator()();

private:
    struct Implementation;
    std::unique_ptr<Implementation> impl_;
};
```

**Source (`MyWidget.cpp`)**
```cpp
#include "MyWidget.hpp"
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>
#include <nui/event_system/observed_value.hpp>

struct MyWidget::Implementation {
    Nui::Observed<std::string> status{"idle"};
    // Other sub-components or state here
};

MyWidget::MyWidget()
    : impl_{std::make_unique<Implementation>()}
{}
MyWidget::~MyWidget() = default;
MyWidget::MyWidget(MyWidget&&) = default;
MyWidget& MyWidget::operator=(MyWidget&&) = default;

Nui::ElementRenderer MyWidget::operator()()
{
    using namespace Nui::Elements;
    using namespace Nui::Attributes;
    using Nui::Elements::div;   // needed: conflicts with global div
    // using Nui::Elements::span;  // add if needed
    // using Nui::Elements::label; // add if needed

    return div{}(
        button{
            onClick = [this](Nui::val) {
                impl_->status = "clicked!";
            }
        }("Click me"),
        impl_->status
    );
}
```

---

### Function Component

Use for small, stateless (or externally-stateful) components. A function component is a free
function that takes an options struct and returns a `Nui::ElementRenderer`. All reactive state
it needs is passed in by the caller. Idiomatic style: split header/source, keep heavy includes
in the source.

**Header (`MyButton.hpp`)**
```cpp
#pragma once
#include <nui/frontend/element_renderer.hpp>
#include <nui/frontend/state_transformer.hpp>
#include <nui/frontend/attributes/impl/attribute.hpp>  // Nui::Attribute
#include <functional>
#include <vector>

namespace MyComponents
{
    // Reification strategies are defined in the source, forward-declared here.
    namespace Detail { struct LabelReify; }

    struct MyButtonOptions {
        // StateTransformer lets the caller pass a plain value, Observed<T>, or combinator.
        Nui::StateTransformer<Detail::LabelReify> label;
        std::vector<Nui::Attribute> attributes = {};
        std::function<void()> onClick = {};
    };

    Nui::ElementRenderer myButton(MyButtonOptions options);
}
```

**Source (`MyButton.cpp`)**
```cpp
#include "MyButton.hpp"
#include <nui/frontend/elements.hpp>
#include <nui/frontend/attributes.hpp>

namespace MyComponents
{
    namespace Detail
    {
        struct LabelReify {
            using type = Nui::ElementRenderer;

            // Plain value:
            static type reify(Nui::StateTransformerBase const&, std::string const& value) {
                using namespace Nui::Elements;
                return span{}(value);
            }

            // Observed<T> or combinator (auto& catches both):
            static type reify(Nui::StateTransformerBase const&, auto& observed) {
                using namespace Nui::Elements;
                return span{}(observed);
            }
        };
    }

    Nui::ElementRenderer myButton(MyButtonOptions options)
    {
        using namespace Nui::Elements;
        using namespace Nui::Attributes;

        auto [labelElement] = options.label.reify();

        return button{
            std::move(options.attributes),
            onClick = [cb = std::move(options.onClick)](Nui::val) {
                if (cb) cb();
            }
        }(std::move(labelElement));
    }
}
```

**Calling a function component:**
```cpp
myButton({ .label = std::string{"Click me"} });          // plain value
myButton({ .label = myObserved });                       // Observed<T>& -- must outlive component
myButton({ .label = sharedObserved });                   // shared_ptr<Observed<T>>
myButton({ .label = weakObserved });                     // weak_ptr<Observed<T>>
myButton({ .label = Nui::observe(a, b).generate(...)});  // combinator
```

---

## `StateTransformer` in Depth

`#include <nui/frontend/state_transformer.hpp>`

`StateTransformer` erases the difference between a plain value, an `Observed<T>` ref, a
`shared_ptr<Observed<T>>`, a `weak_ptr<Observed<T>>`, and a combinator. It is parameterized
by one or more **reification strategies** -- each one describes how to turn the state into
one output (an attribute, a child element, a text node, etc.).

### Reification strategy shape
```cpp
struct MyReify {
    using type = Nui::Attribute;  // or Nui::ElementRenderer, std::string, etc.

    // Overload for plain (non-observed) value:
    static type reify(Nui::StateTransformerBase const&, std::string const& value) {
        return Nui::Attributes::style = fmt::format("color: {};", value);
    }

    // Overload for Observed<T> and combinators (auto& catches both):
    static type reify(Nui::StateTransformerBase const&, auto& observed) {
        return Nui::Attributes::style = Nui::observe(observed).generate(
            [](auto const& v) { return fmt::format("color: {};", v); }
        );
    }
};
```

### Calling `.reify()` -- structured bindings
```cpp
// One binding per strategy, in declaration order:
auto [styleAttr] = options.color.reify();
auto [bgAttr, inputElement] = options.value.reify(); // two strategies
```

### Assigning back (two-way binding)
```cpp
options.value.assign(newValue, Nui::ChangePolicy::Tracked);   // triggers UI update
options.value.assign(newValue, Nui::ChangePolicy::Untracked); // silent, no UI update
```

### Reading the current value
```cpp
auto current = options.value.template value<std::string>();
```

---

## DOM Element References

To get a handle to a rendered DOM element (e.g. to call `.click()` programmatically):

```cpp
#include <nui/frontend/dom/reference.hpp>

auto ref = std::make_shared<std::weak_ptr<Nui::Dom::BasicElement>>();

input{
    reference = [ref](std::weak_ptr<Nui::Dom::BasicElement> element) {
        *ref = std::move(element);
    }
}()

// Later:
if (auto el = ref->lock(); el)
    el->val().call<void>("click", someEvent);
```

---

## JavaScript Interop

### Inline JS via `emscripten::val`
```cpp
#include <nui/frontend/val.hpp>
// Nui::val is a typedef for emscripten::val

auto window = Nui::val::global("window");
window.call<void>("alert", std::string{"Hello from C++"});

auto doc = Nui::val::global("document");
auto el = doc.call<Nui::val>("getElementById", std::string{"my-id"});
```

### Passing C++ callbacks to JS -- `Nui::bind`
```cpp
#include <nui/frontend/utility/functions.hpp>

Nui::val::global("window").call<void>(
    "requestAnimationFrame",
    Nui::bind([](Nui::val timestamp) {
        /* animation frame callback */
    }, std::placeholders::_1)
);
```

For larger JS needs, put the code in a separate `.js` file loaded at startup.

---

## Styling

- **Prefer a separate CSS file** for static styles.
- **Only use inline `style =`** when the value must react to an `Observed`:
  ```cpp
  div{
      style = Nui::observe(obs).generate([](auto const& v) {
          return fmt::format("color: {};", v);
      })
  }()
  ```

---

## Key Headers Reference

```cpp
// Core reactive state
#include <nui/event_system/observed_value.hpp>             // Nui::Observed<T>
#include <nui/event_system/observed_value_combinator.hpp>  // Nui::observe(a,b).generate(...)
#include <nui/event_system/listen.hpp>                     // Nui::listen, Nui::smartListen, Nui::ListenRemover

// Elements and attributes (granular includes in headers; bulk in sources)
#include <nui/frontend/elements.hpp>                       // All HTML elements
#include <nui/frontend/elements/div.hpp>                   // Single element
#include <nui/frontend/elements/nil.hpp>                   // Nui::nil()
#include <nui/frontend/elements/fragment.hpp>              // Nui::Elements::fragment()
#include <nui/frontend/attributes.hpp>                     // All attributes
#include <nui/frontend/attributes/class.hpp>               // Single attribute

// Component building blocks
#include <nui/frontend/element_renderer.hpp>               // Nui::ElementRenderer
#include <nui/frontend/state_transformer.hpp>              // Nui::StateTransformer, Nui::ChangePolicy

// WebAPI event types
#include <nui/frontend/api/event.hpp>                      // Nui::WebApi::Event
#include <nui/frontend/api/mouse_event.hpp>                // Nui::WebApi::MouseEvent
#include <nui/frontend/api/keyboard_event.hpp>             // Nui::WebApi::KeyboardEvent
#include <nui/frontend/api/drag_event.hpp>                 // Nui::WebApi::DragEvent
#include <nui/frontend/api/timer.hpp>                      // JS timers
#include <nui/frontend/api/fetch.hpp>                      // fetch API
#include <nui/frontend/api/console.hpp>                    // Nui::WebApi::Console::log/error

// Utilities
#include <nui/frontend/utility/functions.hpp>              // Nui::bind
#include <nui/frontend/utility/val_conversion.hpp>         // convertToVal / convertFromVal
#include <nui/frontend/dom/reference.hpp>                  // Nui::Dom::BasicElement
#include <nui/frontend/val.hpp>                            // Nui::val (= emscripten::val)
#include <nui/frontend/val_wrapper.hpp>                    // Nui::ValWrapper (base of event types)

// Built-in higher-level components
#include <nui/frontend/components/dialog.hpp>
#include <nui/frontend/components/select.hpp>
#include <nui/frontend/components/table.hpp>
#include <nui/frontend/components/text_input.hpp>

// SVG
#include <nui/frontend/svg.hpp>
#include <nui/frontend/svg_elements.hpp>
#include <nui/frontend/svg_attributes.hpp>
```

---

## Common Pitfalls

1. **Forgetting `()`** -- every element needs a child list, even if empty: `div{}()` not `div{}`.
2. **`long long` index in range** -- the index parameter must be `long long`, not `auto` or `int`.
3. **Elements before/after `Nui::range`** -- not allowed; use `.before()` / `.after()`.
4. **`Nui::observe(...)` not alone** -- the observe+renderer pair must be the sole logical child.
5. **Forgetting `executeActiveEventsImmediately()`** -- in async/timer code, UI won't update without it.
6. **Global name conflicts** -- `div`, `span`, `label` clash with global names; add explicit `using Nui::Elements::div;` etc.
7. **Copying `Observed` when mutating** -- always mutate the original instance.
8. **Dangling ref in `StateTransformer(Observed<T>&)`** -- the observed must outlive the transformer.
9. **Heavy includes in headers** -- keep `elements.hpp` / `attributes.hpp` out of component headers; put them in sources.
10. **Fragments with reactive logic** -- fragments are removed and reinserted as a unit; deferred rerenders will append their contents to the back. Use only for static structure.
11. **Discarding `ListenRemover`** -- `smartListen` is marked `[[nodiscard]]`. Discarding the returned `ListenRemover` immediately removes the listener. Always store it as a member or local that outlives the subscription.
