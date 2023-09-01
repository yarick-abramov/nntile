/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/kernel.hh
 * General info about namespace nntile::kernel
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @author Aleksandr Katrutsa
 * @author Konstantin Sozykin
 * @date 2023-07-20
 * */

#pragma once

#include <nntile/kernel/add_slice.hh>
#include <nntile/kernel/add_slice3.hh>
#include <nntile/kernel/add_fiber.hh>
#include <nntile/kernel/prod_slice.hh>
#include <nntile/kernel/prod_fiber.hh>
#include <nntile/kernel/prod_fiber3.hh>
#include <nntile/kernel/gelu.hh>
#include <nntile/kernel/gelutanh.hh>
#include <nntile/kernel/gelutanh_inplace.hh>
#include <nntile/kernel/dgelu.hh>
#include <nntile/kernel/dgelutanh.hh>
#include <nntile/kernel/drelu.hh>
#include <nntile/kernel/hypot.hh>
#include <nntile/kernel/normalize.hh>
#include <nntile/kernel/prod.hh>
#include <nntile/kernel/randn.hh>
#include <nntile/kernel/relu.hh>
#include <nntile/kernel/relu_forward.hh>
#include <nntile/kernel/relu_backward.hh>
#include <nntile/kernel/subcopy.hh>
#include <nntile/kernel/sumnorm.hh>
#include <nntile/kernel/fill.hh>
#include <nntile/kernel/sum_slice.hh>
#include <nntile/kernel/sum_fiber.hh>
#include <nntile/kernel/norm_slice.hh>
#include <nntile/kernel/pow.hh>
#include <nntile/kernel/maxsumexp.hh>
#include <nntile/kernel/softmax.hh>
#include <nntile/kernel/softmax_inplace.hh>
#include <nntile/kernel/sqrt.hh>
#include <nntile/kernel/sqrt_inplace.hh>
#include <nntile/kernel/maximum.hh>
#include <nntile/kernel/addcdiv.hh>
#include <nntile/kernel/sumprod_slice.hh>
#include <nntile/kernel/sumprod_fiber.hh>
#include <nntile/kernel/logsumexp.hh>
#include <nntile/kernel/total_sum_accum.hh>
#include <nntile/kernel/subtract_indexed_outputs.hh>
#include <nntile/kernel/gelu_backward.hh>
#include <nntile/kernel/gelutanh_backward.hh>
#include <nntile/kernel/add.hh>
#include <nntile/kernel/add_scalar.hh>
#include <nntile/kernel/embedding.hh>
#include <nntile/kernel/embedding_backward.hh>
#include <nntile/kernel/fp32_to_fp16.hh>
#include <nntile/kernel/fp16_to_fp32.hh>
#include <nntile/kernel/mask_scalar.hh>
#include <nntile/kernel/scal.hh>
#include <nntile/kernel/adam_step.hh>
#include <nntile/kernel/transpose.hh>

namespace nntile
{
//! @namespace nntile::kernel
/*! This namespace holds low-level routines for codelets
 * */
namespace kernel
{

} // namespace kernel
} // namespace nntile

