#pragma once

namespace Nui
{
    template <typename ObservedValue>
    class ObservedRange
    {
      public:
        explicit constexpr ObservedRange(ObservedValue& observedValues)
            : observedValue_{observedValues}
        {}

        ObservedValue const& observedValue() const
        {
            return observedValue_;
        }
        ObservedValue& observedValue()
        requires(!std::is_const_v<ObservedValue>)
        {
            return observedValue_;
        }

      private:
        ObservedValue& observedValue_;
    };

    template <typename ObservedValue>
    ObservedRange<ObservedValue> range(ObservedValue& observedValues)
    {
        return ObservedRange<ObservedValue>{observedValues};
    }

    template <typename ObservedValue>
    ObservedRange<const ObservedValue> range(ObservedValue const& observedValues)
    {
        return ObservedRange<const ObservedValue>{observedValues};
    }
}