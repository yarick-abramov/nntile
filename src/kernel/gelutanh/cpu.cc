/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/kernel/gelutanh/cpu.cc
 * Approximate GeLU operation on CPU based on tanh function
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-07-09
 * */

#include "nntile/kernel/gelutanh/cpu.hh"
#include <cmath>

namespace nntile
{
namespace kernel
{
namespace gelutanh
{

template<typename T>
void cpu(Index nelems, const T *src, T *dst)
    noexcept
//! Approximate GeLU operation on CPU
/*! Applies the following approximation of the GeLU function:
 * GeLU(z) \approx AGeLU(z)
 * AGeLU(z) \approx 0.5z(1+tanh(sqrt(2/pi)(z+0.044715z^3))),
 * which is actually implemented as
 * f(z) = -2 sqrt(2/pi) z (1+0.044715z^2)
 * AGeLU(z) = z / (1+exp(f(z))
 *
 * @params[in] nelems: Number of elements in a buffer
 * @params[in] src: Input buffer to apply GeLU
 * @params[out] dst: Output buffer to apply GeLU
 * */
{
    // Constants
    constexpr T pi = 3.141592653589793238462643383279502884L,
        one = 1, pt5 = 0.5, f1 = T{0.044715};
    // Square root is not constexpr by standard, proceed with a static const
    static const T sqrt_pi = std::sqrt(pi), sqrt_2 = std::sqrt(T{2}),
        f2 = sqrt_2/sqrt_pi, f3 = -T{2}*f2, f4 = f3*f1;
    for(Index i = 0; i < nelems; ++i)
    {
        T z = src[i];
        //T y = z * (f3 + f4*z*z);
        //dst[i] = z / (one+std::exp(y));
        T y1 = f4 * z * z;
        T y2 = f3 + y1;
        T c = y1 - (y2-f3);
        y2 *= z;
        c *= z;
        T y3 = one + std::exp(c)*std::exp(y2);
        dst[i] = z / y3;
    }
}

// Explicit instantiation
template
void cpu<fp32_t>(Index nelems, const fp32_t *src, fp32_t *dst)
    noexcept;

template
void cpu<fp64_t>(Index nelems, const fp64_t *src, fp64_t *dst)
    noexcept;

} // namespace gelutanh
} // namespace kernel
} // namespace nntile

