/*! @copyright (c) 2022-present Skolkovo Institute of Science and Technology
 *                              (Skoltech), Russia. All rights reserved.
 *                 2023-present Artificial Intelligence Research Institute
 *                              (AIRI), Russia. All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/tensor/lars_tiled_step.hh
 * Fused LarsTiled step operation for Tensor<T>
 *
 * @version 1.1.0
 * */

#pragma once

#include <nntile/tensor/tensor.hh>

namespace nntile::tensor
{

template<typename T>
void lars_tiled_step_async(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay,
    Scalar lars_coefficient, const Tensor<T> &grad, const Tensor<T> &momentum_buffer, const Tensor<T> &p);

                   
template<typename T>
void lars_tiled_step(Index num_iter, Index num_steps, Scalar gamma_0, Scalar momentum, Scalar weight_decay,
    Scalar lars_coefficient, const Tensor<T> &grad, const Tensor<T> &momentum_buffer, const Tensor<T> &p);

} // namespace nntile::tensor
