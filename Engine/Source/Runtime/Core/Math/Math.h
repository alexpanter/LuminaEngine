#pragma once

#include <eastl/type_traits.h>
#include <random>
#include "eastl/utility.h"
#include <glm/glm.hpp>

#include "Platform/GenericPlatform.h"

namespace Lumina::Math
{
    constexpr int NextPowerOfTwo(int v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }
    
    template<typename T>
    [[nodiscard]] constexpr T Max(const T& First, const T& Second)
    {
        return glm::max(First, Second);
    }

    template<typename T>
    [[nodiscard]] constexpr T Min(const T& First, const T& Second)
    {
        return glm::min(First, Second);
    }

    template<typename T>
    [[nodiscard]] constexpr T Clamp(const T& A, const T& First, const T& Second)
    {
        return glm::clamp(A,First,Second);
    }

    template<typename T>
    [[nodiscard]] constexpr T Abs(const T& A)
    {
        return glm::abs(A);
    }

    template<typename T>
    [[nodiscard]] constexpr T Floor(const T& A)
    {
        return glm::floor(A);
    }

    template<typename T>
    [[nodiscard]] constexpr T Pow(const T& A, const T& B)
    {
        return glm::pow(A,B);
    }

    template<typename T>
    [[nodiscard]] constexpr T Lerp(const T& A, const T& B, float Alpha)
    {
        return A + (B - A) * Alpha;
    }
    
    [[nodiscard]] constexpr uint64 CountTrailingZeros64(uint64 Value)
    {
        if (Value == 0)
        {
            return 64;
        }
        uint64 Result = 0;
        while ((Value & 1) == 0)
        {
            Value >>= 1;
            ++Result;
        }
        return Result;
    }
    
    template<typename T>
    requires(eastl::is_integral_v<T> && eastl::is_unsigned_v<T> && (sizeof(T) <= 4))
    [[nodiscard]] T RandRange(T First, T Second)
    {
        if (First > Second)
        {
            eastl::swap(First, Second);
        }
        
        thread_local std::mt19937 Random(std::random_device{}());
        std::uniform_int_distribution<T> Distribution(First, Second);
    
        return Distribution(Random);
    }

    RUNTIME_API glm::quat FindLookAtRotation(const glm::vec3& Target, const glm::vec3& From);
}
