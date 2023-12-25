#pragma once

namespace RenderEngine
{
    template<class... Ts>
    struct overloaded : Ts... { using Ts::operator()...; };
}