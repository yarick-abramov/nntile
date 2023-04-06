/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file include/nntile/tile/gelutanh_backward.hh
 * Backward approximate GeLU operation for Tile<T>
 *
 * @version 1.0.0
 * @author Aleksandr Katrutsa
 * @author Aleksandr Mikhalev
 * @date 2023-04-05
 * */

#pragma once

#include <nntile/tile/tile.hh>

namespace nntile
{
namespace tile
{

// Asynchronous tile-wise backward approximate GeLU operation
template<typename T>
void gelutanh_backward_async(const Tile<T> &x, const Tile<T> &dy,
        const Tile<T> &dx);

// Blocking version of tile-wise backward approximate GeLU operation
template<typename T>
void gelutanh_backward(const Tile<T> &x, const Tile<T> &dy, const Tile<T> &dx);

} // namespace tile
} // namespace nntile

