/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/tensor/lars_tiled_step.cc
 * Fuse LARS step operation for Tensor<T>
 *
 * @version 1.1.0
 * */

#include "nntile/tensor/lars_tiled_step.hh"
#include "nntile/starpu/lars_tiled_step.hh"

namespace nntile::tensor
{

//! Asynchronous tensor-wise fuse LARS step
template<typename T>
void lars_tiled_step_async(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<T> &grad, const Tensor<T> &momentum_buffer, const Tensor<T> &p)
{
    if (p.matrix_shape != grad.matrix_shape)
    {
        throw std::runtime_error("Parameter shape is not equal to gradient shape");
    }

    if (p.matrix_shape != momentum_buffer.matrix_shape)
    {
        throw std::runtime_error("Parameter shape is not equal to momentum_buffer shape");
    }

    int mpi_size = starpu_mpi_world_size();
    int mpi_rank = starpu_mpi_world_rank();

    for(Index i = 0; i < p.grid.nelems; ++i)
    {
        // Get handle for corresponding tiles of src and dst
        auto p_tile_handle = p.get_tile_handle(i);
        auto grad_tile_handle = grad.get_tile_handle(i);
        auto momentum_buffer_tile_handle = momentum_buffer.get_tile_handle(i);
        // MPI rank of the destination tile
        int p_tile_rank = p_tile_handle.mpi_get_rank();
        int grad_tile_rank = grad_tile_handle.mpi_get_rank();
        int momentum_buffer_tile_rank = momentum_buffer_tile_handle.mpi_get_rank();
        // Transfer data
        grad_tile_handle.mpi_transfer(p_tile_rank, mpi_rank);
        momentum_buffer_tile_handle.mpi_transfer(p_tile_rank, mpi_rank);
        // Execute only on destination node
        if(mpi_rank == p_tile_rank)
        {
            auto traits = p.get_tile_traits(i);
            starpu::lars_tiled_step::submit<T>(num_iter, traits.nelems, num_steps, gamma_0, momentum, weight_decay, lars_coefficient,
                                         grad_tile_handle, momentum_buffer_tile_handle, p_tile_handle);
        }
        // Flush cache for the output tile on every node
        p_tile_handle.mpi_flush();
    }
}

//! Blocking version of tensor-wise addcdiv operation
template<typename T>
void lars_tiled_step(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
    const Tensor<T> &grad, const Tensor<T> &momentum_buffer, const Tensor<T> &p)
{
    lars_tiled_step_async<T>(num_iter, num_steps, gamma_0, momentum, weight_decay, lars_coefficient, grad, momentum_buffer, p);
    starpu_task_wait_for_all();
    starpu_mpi_wait_for_all(MPI_COMM_WORLD);
}

// Explicit instantiation
template
void lars_tiled_step_async<fp32_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_t> &grad, const Tensor<fp32_t> &momentum_buffer, const Tensor<fp32_t> &p);

template
void lars_tiled_step_async<fp32_fast_tf32_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_fast_tf32_t> &grad, const Tensor<fp32_fast_tf32_t> &momentum_buffer, const Tensor<fp32_fast_tf32_t> &p);

template
void lars_tiled_step_async<fp32_fast_fp16_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_fast_fp16_t> &grad, const Tensor<fp32_fast_fp16_t> &momentum_buffer, const Tensor<fp32_fast_fp16_t> &p);

template
void lars_tiled_step_async<fp32_fast_bf16_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_fast_bf16_t> &grad, const Tensor<fp32_fast_bf16_t> &momentum_buffer, const Tensor<fp32_fast_bf16_t> &p);

template
void lars_tiled_step_async<fp64_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp64_t> &grad, const Tensor<fp64_t> &momentum_buffer, const Tensor<fp64_t> &p);

template
void lars_tiled_step_async<bf16_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<bf16_t> &grad, const Tensor<bf16_t> &momentum_buffer, const Tensor<bf16_t> &p);

// Explicit instantiation
template
void lars_tiled_step<fp32_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_t> &grad, const Tensor<fp32_t> &momentum_buffer, const Tensor<fp32_t> &p);

template
void lars_tiled_step<fp32_fast_tf32_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_fast_tf32_t> &grad, const Tensor<fp32_fast_tf32_t> &momentum_buffer, const Tensor<fp32_fast_tf32_t> &p);

template
void lars_tiled_step<fp32_fast_fp16_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_fast_fp16_t> &grad, const Tensor<fp32_fast_fp16_t> &momentum_buffer, const Tensor<fp32_fast_fp16_t> &p);

template
void lars_tiled_step<fp32_fast_bf16_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp32_fast_bf16_t> &grad, const Tensor<fp32_fast_bf16_t> &momentum_buffer, const Tensor<fp32_fast_bf16_t> &p);

template
void lars_tiled_step<fp64_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<fp64_t> &grad, const Tensor<fp64_t> &momentum_buffer, const Tensor<fp64_t> &p);

template
void lars_tiled_step<bf16_t>(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay, Scalar lars_coefficient,
                           const Tensor<bf16_t> &grad, const Tensor<bf16_t> &momentum_buffer, const Tensor<bf16_t> &p);

} // namespace nntile::tensor