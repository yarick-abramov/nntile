/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/tensor/biasprod.hh
 * Bias-like product operation for Tensor<T>
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-04-19
 * */

#pragma once

#include <nntile/tensor/tensor.hh>

namespace nntile
{
namespace tensor
{

// Tensor-wise biasprod operation
template<typename T>
void biasprod_async(const Tensor<T> &src, const Tensor<T> &dst, Index axis);

// Tensor-wise biasprod operation
template<typename T>
void biasprod(const Tensor<T> &src, const Tensor<T> &dst, Index axis);

} // namespace tensor
} // namespace nntile

