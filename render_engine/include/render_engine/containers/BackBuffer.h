#pragma once

#include <vector>

namespace RenderEngine
{
    template<typename T>
    class BackBuffer
    {
    public:
        explicit BackBuffer(size_t size)
            :_back_buffer()
        {
            _back_buffer.resize(size);
        }

        BackBuffer(const BackBuffer&) = delete;
        BackBuffer(BackBuffer&&) = default;
        BackBuffer& operator=(const BackBuffer&) = delete;
        BackBuffer& operator=(BackBuffer&&) = default;

        typename std::vector<T>::iterator begin() { return _back_buffer.begin(); }
        typename std::vector<T>::iterator end() { return _back_buffer.end(); }

        const T& operator[](size_t frame_number) const
        {
            return _back_buffer[frame_number % _back_buffer.size()];
        }
        T& operator[](size_t frame_number)
        {
            return _back_buffer[frame_number % _back_buffer.size()];
        }
        size_t size() const { return _back_buffer.size(); }
    private:
        std::vector<T> _back_buffer;
    };
}
