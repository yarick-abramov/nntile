/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/starpu/flash_softmax_gemm_backward_sumprod_slice.hh
 * Flash Attention backward to get sumprod_slice result
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-11-12
 * */

#pragma once

#include <nntile/base_types.hh>
#include <nntile/starpu/config.hh>

namespace nntile
{
namespace starpu
{
namespace flash_softmax_gemm_backward_sumprod_slice
{

//! Structure for arguments
struct args_t
{
    Index seq;
    Index head;
    Index batch;
};

#ifdef NNTILE_USE_CBLAS
template<typename T>
void cpu(void *buffers[], void *cl_args)
    noexcept;
#endif // NNTILE_USE_CBLAS

#ifdef NNTILE_USE_CUDA
template<typename T>
void cuda(void *buffers[], void *cl_args)
    noexcept;
#endif // NNTILE_USE_CUDA

extern Codelet codelet_fp32, codelet_fp64, codelet_fp32_fast_tf32;

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
constexpr Codelet *codelet<fp64_t>()
{
    return &codelet_fp64;
}

void init();

void restrict_where(uint32_t where);

void restore_where();

template<typename T>
void submit(Index seq, Index head, Index batch, Handle K, Handle Q,
        Handle mask, Handle maxsumexp, Handle dA, Handle V, Handle dV,
        Handle sumprod_slice, Handle tmp, Handle tmp_grad, int redux=0,
        int fp32_fast_tf32=0);

} // namespace flash_softmax_gemm_backward_sumprod_slice
} // namespace starpu
} // namespace nntile

