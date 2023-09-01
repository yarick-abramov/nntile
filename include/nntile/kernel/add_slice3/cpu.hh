/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/kernel/add_slice3/cpu.hh
 * Per-element addition of a tensor and a broadcasted slice on CPU
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-07-03
 * */

#pragma once

#include <nntile/base_types.hh>

namespace nntile
{
namespace kernel
{
namespace add_slice3
{

// Per-element addition of a tensor and a broadcasted slice on CPU
template<typename T>
void cpu(Index m, Index n, Index k, T alpha, const T *src1, T beta,
        const T *src2, T *dst)
    noexcept;

} // namespace add_slice3
} // namespace kernel
} // namespace nntile

