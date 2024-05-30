#pragma once

#include <concepts>

namespace RenderEngine
{
    template<class T>
    concept Referencable =
        std::copyable<T> == false
        && std::movable<T> == false;

    template<Referencable T>
    class RefObj
    {
    public:
        RefObj(T& obj)
            : _obj(obj)
        {}

        T& operator->() { return _obj; }
        const T& operator->() const { return _obj; }

        T* operator*() { return _obj; }
        const T* operator*() const { return _obj; }
    private:
        T& _obj;
    };
}