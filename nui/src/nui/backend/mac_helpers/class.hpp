#pragma once

#include "mac_includes.hpp"
#include "type_encodings.hpp"

namespace Nui::MacOs
{
    template <typename T>
    class ClassWrapper
    {
      public:
        static auto createInstance(std::string const& name)
        {
            auto cls = objc_getClass(name.c_str());
            auto inst = class_createInstance(cls, sizeof(T) /* - sizeof(Class)?*/);
            // here or offset by sizeof(Class)?
            new (inst) T(cls);
            return inst;
        }

        static auto createNsObjectClassPair(std::string const& name)
        {
            return ClassWrapper<T>{objc_allocateClassPair(reinterpret_cast<Class>("NSObject"_cls), name.c_str(), 0)};
        }

        Class native() const noexcept
        {
            return m_class;
        }

        ClassWrapper(Class cls)
            : m_class{cls}
        {}

        void addProtocol(std::string const& name)
        {
            class_addProtocol(m_class, objc_getProtocol(name.c_str()));
        }

        void registerClassPair()
        {
            objc_registerClassPair(m_class);
        }

        template <typename FunctionT>
        void addMethod(std::string const& name, FunctionT func)
        {
            class_addMethod(
                m_class, sel_registerName(name.c_str()), reinterpret_cast<IMP>(func), encodeType<FunctionT>().c_str());
        }

      private:
        Class m_class;
    };
}