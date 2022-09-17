#pragma once

namespace Nui
{
    template <typename ObservedValue>
    class ObservedRange
    {
      public:
        constexpr ObservedRange(ObservedValue& observedValues)
            : observedValue_{observedValues}
        {}

        ObservedValue& observedValue() const
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
}