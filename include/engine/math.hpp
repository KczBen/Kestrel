#pragma once

#include <array>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <numeric>
namespace Kestrel {
template <std::size_t N, typename T = double> struct Vec {
    std::array<T, N> v{};

    constexpr Vec() = default;
    template <typename... Args>
        requires(sizeof...(Args) == N && (std::convertible_to<Args, T> && ...))
    constexpr Vec(Args... args) noexcept : v{static_cast<T>(args)...} {}

    constexpr auto operator[](this auto&& self, std::size_t i) noexcept -> decltype(auto) { return (self.v[i]); }

    constexpr auto x(this auto&& self) noexcept -> decltype(auto)
        requires(N >= 1)
    {
        return (self.v[0]);
    }
    constexpr auto y(this auto&& self) noexcept -> decltype(auto)
        requires(N >= 2)
    {
        return (self.v[1]);
    }
    constexpr auto z(this auto&& self) noexcept -> decltype(auto)
        requires(N >= 3)
    {
        return (self.v[2]);
    }
    constexpr auto w(this auto&& self) noexcept -> decltype(auto)
        requires(N >= 4)
    {
        return (self.v[3]);
    }

    constexpr auto operator==(const Vec&) const -> bool = default;

    constexpr auto operator+=(const Vec& o) noexcept -> Vec& {
        for (std::size_t i = 0; i < N; ++i) {
            v[i] += o.v[i];
        }

        return *this;
    }
    constexpr auto operator-=(const Vec& o) noexcept -> Vec& {
        for (std::size_t i = 0; i < N; ++i) {
            v[i] -= o.v[i];
        }
        return *this;
    }
    friend constexpr auto operator+(Vec a, const Vec& b) noexcept -> Vec { return a += b; }
    friend constexpr auto operator-(Vec a, const Vec& b) noexcept -> Vec { return a -= b; }

    [[nodiscard]] constexpr auto dot(const Vec& o) const noexcept -> T {
        return std::inner_product(v.begin(), v.end(), o.v.begin(), T{});
    }

    [[nodiscard]] auto len() const noexcept -> double { return std::sqrt(static_cast<double>(dot(*this))); }
};

template <typename T> using Vec2 = Vec<2, T>;
template <typename T> using Vec3 = Vec<3, T>;
template <typename T> using Vec4 = Vec<4, T>;

inline constexpr Vec3<double> WORLD_FORWARD{0, 0, 1};   // NOLINT
inline constexpr Vec3<double> WORLD_UP{0, 1, 0};        // NOLINT
inline constexpr Vec3<double> WORLD_RIGHT{1, 0, 0};     // NOLINT
inline constexpr Vec3<double> CAMERA_FORWARD{0, 0, -1}; // NOLINT

struct Quat {
    double x = 1.0;
    double y = 0.0;
    double z = 0.0;
    double w = 0.0;
};
} // namespace Kestrel
