#pragma once

#include <concepts>

namespace RenderEngine
{
    template<class T>
    concept Referencable =
        std::copyable<T> == false
        && std::movable<T> == false;

    /*
    * TODO: Need another object called Owner (similar concept like gsl::owner but still different).
    * RefObj can be created only by Owners
    * RefObj might be copyable but cannot be moveable
    *
    * A class can store:
    * RefObj: Instead of reference
    * Owner: When the object might be referenced by another object
    * shared_ptr: When the ownership is shared
    * unique_ptr: When it is nullable
    * raw_ptr: reference to a unique_ptr
    * Value-Object
    *
    * A class cannot store:
    * reference: It is awefull when the referenced object is moved
    */
    template<Referencable T>
    class RefObj
    {
    public:
        RefObj(T& obj)
            : _obj(obj)
        {}

        T* operator->() { return &_obj; }
        const T* operator->() const { return &_obj; }

        T& operator*() { return _obj; }
        const T& operator*() const { return _obj; }
    private:
        T& _obj;
    };
}