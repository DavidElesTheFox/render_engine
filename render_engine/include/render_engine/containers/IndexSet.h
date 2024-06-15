#pragma once

#include <type_traits>

#include <cassert>

namespace RenderEngine
{
    template<typename BaseType>
        requires std::is_integral_v<BaseType>
    class IndexSet
    {
    public:
        static constexpr const uint32_t MaxCount = sizeof(BaseType) * 8;
        IndexSet() = default;

        IndexSet(const IndexSet&) = default;
        IndexSet(IndexSet&&) = default;

        IndexSet& operator=(const IndexSet&) = default;
        IndexSet& operator=(IndexSet&&) = default;

        bool insert(uint32_t index)
        {
            const bool result = contains(index) == false;
            assert(index < MaxCount);
            _flags |= (1 << index);
            return result;
        }

        bool insert(const IndexSet& o)
        {
            const bool result = _flags == o._flags;
            _flags |= o._flags;
            return result;
        }

        void erase(uint32_t index)
        {
            assert(index < MaxCount);
            _flags &= ~(1 << index);
        }
        bool contains(uint32_t index) const
        {
            assert(index < MaxCount);
            return _flags & (1 << index);
        }

        IndexSet<BaseType> negate() const
        {
            return IndexSet(~_flags);
        }

        bool empty() const
        {
            return _flags == 0;
        }

    private:
        explicit IndexSet(BaseType flags)
            : _flags(flags)
        {}
        BaseType _flags{ BaseType{0} };
    };
}