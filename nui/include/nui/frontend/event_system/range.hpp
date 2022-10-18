#pragma once

namespace Nui
{
    template <typename ObservedValue>
    class ObservedRange
    {
      public:
        constexpr ObservedRange(ObservedValue const& observedValues)
            : observedValue_{observedValues}
        {}

        ObservedValue const& observedValue() const
        {
            return observedValue_;
        }

      private:
        ObservedValue const& observedValue_;
    };

    template <typename ObservedValue>
    ObservedRange<ObservedValue> range(ObservedValue const& observedValues)
    {
        return ObservedRange<ObservedValue>{observedValues};
    }
}