/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/starpu/lars_tiled_step.cc
 * Per-element addcdiv operation of StarPU buffers
 *
 * @version 1.1.0
 * */

#ifndef STARPU_SIMGRID
#include "nntile/kernel/lars_tiled_step.hh"
#endif // STARPU_SIMGRID
#include "nntile/starpu/lars_tiled_step.hh"
#include <cstdlib>

//! StarPU wrappers for one step of LarsTiled optimizer
namespace nntile::starpu::lars_tiled_step
{

//! Apply LarsTiled step on StarPU buffers on CPU
template<typename T>
void cpu(void *buffers[], void *cl_args)
    noexcept
{
#ifndef STARPU_SIMGRID // Run the code only if this is not a simulation
    // Get arguments
    auto args = reinterpret_cast<args_t *>(cl_args);
    // Get interfaces
    auto interfaces = reinterpret_cast<VariableInterface **>(buffers);
    T *grad = interfaces[0]->get_ptr<T>();
    T *momentum_buffer = interfaces[1]->get_ptr<T>();
    T *p = interfaces[2]->get_ptr<T>();
    // Launch kernel
    kernel::lars_tiled_step::cpu<T>(args->num_iter, args->num_elems, args->num_steps, args->gamma_0,
        args->momentum, args->weight_decay, args->lars_coefficient,
        grad, momentum_buffer, p);
#endif // STARPU_SIMGRID
}

#ifdef NNTILE_USE_CUDA
//! Apply LarsTiled step operation on StarPU buffer on CUDA
template<typename T>
void cuda(void *buffers[], void *cl_args)
    noexcept
{
#ifndef STARPU_SIMGRID // Run the code only if this is not a simulation
    // Get arguments
    auto args = reinterpret_cast<args_t *>(cl_args);
    // Get interfaces
    auto interfaces = reinterpret_cast<VariableInterface **>(buffers);
    T *grad = interfaces[0]->get_ptr<T>();
    T *momentum_buffer = interfaces[1]->get_ptr<T>();
    T *p = interfaces[2]->get_ptr<T>();
    // Get CUDA stream
    cudaStream_t stream = starpu_cuda_get_local_stream();
    // Launch kernel
    kernel::lars_tiled_step::cuda<T>(stream, args->num_iter, args->num_elems, args->num_steps, args->gamma_0,
        args->momentum, args->weight_decay, args->lars_coefficient,
        grad, momentum_buffer, p);
#endif // STARPU_SIMGRID
}
#endif // NNTILE_USE_CUDA

Codelet codelet_fp32, codelet_fp64, codelet_fp32_fast_tf32, codelet_bf16,
        codelet_fp32_fast_fp16, codelet_fp32_fast_bf16;

void init()
{
    codelet_fp32.init("nntile_lars_tiled_step_fp32",
            nullptr,
            {cpu<fp32_t>},
#ifdef NNTILE_USE_CUDA
            {cuda<fp32_t>}
#else // NNTILE_USE_CUDA
            {}
#endif // NNTILE_USE_CUDA
            );

    codelet_bf16.init("nntile_lars_tiled_step_bf16",
            nullptr,
            {cpu<bf16_t>},
#ifdef NNTILE_USE_CUDA
            {cuda<bf16_t>}
#else // NNTILE_USE_CUDA
            {}
#endif // NNTILE_USE_CUDA
            );

    codelet_fp32_fast_tf32.init("nntile_lars_tiled_step_fp32_fast_tf32",
            nullptr,
            {cpu<fp32_t>},
#ifdef NNTILE_USE_CUDA
            {cuda<fp32_t>}
#else // NNTILE_USE_CUDA
            {}
#endif // NNTILE_USE_CUDA
            );

    codelet_fp32_fast_fp16.init("nntile_lars_tiled_step_fp32_fast_fp16",
            nullptr,
            {cpu<fp32_t>},
#ifdef NNTILE_USE_CUDA
            {cuda<fp32_t>}
#else // NNTILE_USE_CUDA
            {}
#endif // NNTILE_USE_CUDA
            );

    codelet_fp32_fast_bf16.init("nntile_lars_tiled_step_fp32_fast_bf16",
            nullptr,
            {cpu<fp32_t>},
#ifdef NNTILE_USE_CUDA
            {cuda<fp32_t>}
#else // NNTILE_USE_CUDA
            {}
#endif // NNTILE_USE_CUDA
            );

    codelet_fp64.init("nntile_lars_tiled_step_fp64",
            nullptr,
            {cpu<fp64_t>},
#ifdef NNTILE_USE_CUDA
            {cuda<fp64_t>}
#else // NNTILE_USE_CUDA
            {}
#endif // NNTILE_USE_CUDA
            );
}

void restrict_where(uint32_t where)
{
    codelet_fp32.restrict_where(where);
    codelet_bf16.restrict_where(where);
    codelet_fp32_fast_tf32.restrict_where(where);
    codelet_fp32_fast_fp16.restrict_where(where);
    codelet_fp32_fast_bf16.restrict_where(where);
    codelet_fp64.restrict_where(where);
}

void restore_where()
{
    codelet_fp32.restore_where();
    codelet_bf16.restore_where();
    codelet_fp32_fast_tf32.restore_where();
    codelet_fp32_fast_fp16.restore_where();
    codelet_fp32_fast_bf16.restore_where();
    codelet_fp64.restore_where();
}

template<typename T>
void submit(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0, Scalar momentum,
    Scalar weight_decay, Scalar lars_coefficient,
    Handle grad, Handle momentum_buffer, Handle p)
{
    // Codelet arguments
    args_t* args = (args_t*)std::malloc(sizeof(*args));
    args->num_iter = num_iter;
    args->num_elems = num_elems;
    args->num_steps = num_steps;
    args->gamma_0 = gamma_0;
    args->momentum = momentum;
    args->weight_decay = weight_decay;
    args->lars_coefficient = lars_coefficient;
    //double nflops = 5 * nelems;
    // Submit task
    enum starpu_data_access_mode moments_mode;
    if (num_iter == 1)
    {
        moments_mode = STARPU_W;
    }
    else
    {
        moments_mode = STARPU_RW;
    }
    int ret = starpu_task_insert(codelet<T>(),
            STARPU_R, static_cast<starpu_data_handle_t>(grad),
            moments_mode, static_cast<starpu_data_handle_t>(momentum_buffer),
            STARPU_RW, static_cast<starpu_data_handle_t>(p),
            STARPU_CL_ARGS, args, sizeof(*args),
            0);
    // Check submission
    if(ret != 0)
    {
        throw std::runtime_error("Error in lars_tiled_step task submission");
    }
}

// Explicit instantiaion
template
void submit<fp32_t>(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0, Scalar momentum,
    Scalar weight_decay, Scalar lars_coefficient,
    Handle grad, Handle momentum_buffer, Handle p);

template
void submit<fp32_fast_tf32_t>(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0, Scalar momentum,
    Scalar weight_decay, Scalar lars_coefficient,
    Handle grad, Handle momentum_buffer, Handle p);

template
void submit<fp32_fast_fp16_t>(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0, Scalar momentum,
    Scalar weight_decay, Scalar lars_coefficient,
    Handle grad, Handle momentum_buffer, Handle p);

template
void submit<fp32_fast_bf16_t>(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0, Scalar momentum,
    Scalar weight_decay, Scalar lars_coefficient,
    Handle grad, Handle momentum_buffer, Handle p);

template
void submit<fp64_t>(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0, Scalar momentum,
    Scalar weight_decay, Scalar lars_coefficient,
    Handle grad, Handle momentum_buffer, Handle p);

template
void submit<bf16_t>(Index num_iter, Index num_elems, Index num_steps, Scalar gamma_0, Scalar momentum,
    Scalar weight_decay, Scalar lars_coefficient,
    Handle grad, Handle momentum_buffer, Handle p);

} // namespace nntile::starpu::lars_tiled_step
