/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/starpu/lars_tiled_step.hh
 * LarsTiled step with StarPU buffers
 *
 * @version 1.1.0
 * */

#pragma once

#include <nntile/base_types.hh>
#include <nntile/starpu/config.hh>

namespace nntile::starpu::lars_tiled_step
{

//! Structure for arguments
struct args_t
{
    Index num_iter;
    Index num_elems;
    Index num_steps;
    Scalar gamma_0;
    Scalar momentum;
    Scalar weight_decay;
    Scalar lars_coefficient;
};

// Apply LarsTiled step to StarPU buffers on CPU
template<typename T>
void cpu(void *buffers[], void *cl_args)
    noexcept;

#ifdef NNTILE_USE_CUDA
// Apply LarsTiled step of StarPU buffers on CUDA
template<typename T>
void cuda(void *buffers[], void *cl_args)
    noexcept;
#endif // NNTILE_USE_CUDA

extern Codelet codelet_fp32, codelet_fp64, codelet_fp32_fast_tf32, codelet_bf16,
               codelet_fp32_fast_fp16, codelet_fp32_fast_bf16;

template<typename T>
constexpr Codelet *codelet()
{
    throw std::runtime_error("Non-supported type");
    return nullptr;
}

template<>
constexpr Codelet *codelet<fp32_t>()
{
    return &codelet_fp32;
}

template<>
constexpr Codelet *codelet<bf16_t>()
{
    return &codelet_bf16;
}

template<>
constexpr Codelet *codelet<fp32_fast_tf32_t>()
{
    return &codelet_fp32_fast_tf32;
}

template<>
constexpr Codelet *codelet<fp32_fast_fp16_t>()
{
    return &codelet_fp32_fast_fp16;
}

template<>
constexpr Codelet *codelet<fp32_fast_bf16_t>()
{
    return &codelet_fp32_fast_bf16;
}

template<>
constexpr Codelet *codelet<fp64_t>()
{
    return &codelet_fp64;
}

void init();

void restrict_where(uint32_t where);

void restore_where();

template<typename T>
void submit(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0,
            Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
            Handle grad, Handle momentum_buffer, Handle p);

} // namespace nntile::starpu::lars_tiled_step