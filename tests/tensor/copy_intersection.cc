/*! @copyright (c) 2022-2022 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file tests/tensor/copy_intersection.cc
 * Copy operation for Tensor<T>
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2022-09-13
 * */

#include "nntile/tensor/copy_intersection.hh"
#include "nntile/starpu/subcopy.hh"
#include "../testing.hh"
#include "../starpu/common.hh"

using namespace nntile;
using namespace nntile::tensor;

template<typename T>
void check(const std::vector<Index> &shape,
        const std::vector<Index> &src_offset,
        const std::vector<Index> &src_shape,
        const std::vector<Index> &src_basetile,
        const std::vector<Index> &dst_offset,
        const std::vector<Index> &dst_shape,
        const std::vector<Index> &dst_basetile)
{
    // Barrier to wait for cleanup of previously used tags
    starpu_mpi_barrier(MPI_COMM_WORLD);
    // Some preparation
    starpu_mpi_tag_t last_tag = 0;
    int mpi_size = starpu_mpi_world_size();
    int mpi_rank = starpu_mpi_world_rank();
    // Traits of source and destination tensors
    tile::TileTraits traits(shape);
    TensorTraits src_traits(src_shape, src_basetile),
                 dst_traits(dst_shape, dst_basetile);
    // Distributions for source and destination tiles
    Index src_ntiles = src_traits.grid.nelems;
    Index dst_ntiles = dst_traits.grid.nelems;
    std::vector<int> src_distr(src_ntiles), dst_distr(dst_ntiles);
    for(Index i = 0; i < src_ntiles; ++i)
    {
        src_distr[i] = (i+1) % mpi_size;
    }
    for(Index i = 0; i < dst_ntiles; ++i)
    {
        dst_distr[i] = (i*i+2) % mpi_size;
    }
    // Init source tensor
    Tensor<T> src(src_traits, src_distr, last_tag);
    for(Index i = 0; i < src_ntiles; ++i)
    {
        if(src_distr[i] == mpi_rank)
        {
            auto tile_handle = src.get_tile_handle(i);
            auto tile_local = tile_handle.acquire(STARPU_W);
            T *tile_local_ptr = reinterpret_cast<T *>(tile_local.get_ptr());
            auto tile_traits = src.get_tile_traits(i);
            auto tile_index = src.grid.linear_to_index(i);
            for(Index j = 0; j < src.ndim; ++j)
            {
                tile_index[j] *= src.basetile_shape[j];
                tile_index[j] += src_offset[j];
            }
            for(Index j = 0; j < tile_traits.nelems; ++j)
            {
                auto global_index = tile_traits.linear_to_index(j);
                for(Index k = 0; k < src.ndim; ++k)
                {
                    global_index[k] += tile_index[k];
                }
                tile_local_ptr[j] = T(traits.index_to_linear(global_index));
            }
            tile_local.release();
        }
    }
    // Define destination tensor and init it with -1
    Tensor<T> dst(dst_traits, dst_distr, last_tag);
    for(Index i = 0; i < dst_ntiles; ++i)
    {
        if(dst_distr[i] == mpi_rank)
        {
            auto tile_handle = dst.get_tile_handle(i);
            auto tile_local = tile_handle.acquire(STARPU_W);
            T *tile_local_ptr = reinterpret_cast<T *>(tile_local.get_ptr());
            auto tile_traits = dst.get_tile_traits(i);
            for(Index j = 0; j < tile_traits.nelems; ++j)
            {
                tile_local_ptr[j] = T{-1};
            }
            tile_local.release();
        }
    }
    // Copy
    copy_intersection<T>(src, src_offset, dst, dst_offset);
    // Check copy
    for(Index i = 0; i < dst_ntiles; ++i)
    {
        if(dst_distr[i] == mpi_rank)
        {
            auto tile_handle = dst.get_tile_handle(i);
            auto tile_local = tile_handle.acquire(STARPU_R);
            T *tile_local_ptr = reinterpret_cast<T *>(tile_local.get_ptr());
            auto tile_traits = dst.get_tile_traits(i);
            auto tile_index = dst.grid.linear_to_index(i);
            for(Index j = 0; j < dst.ndim; ++j)
            {
                tile_index[j] *= dst.basetile_shape[j];
                tile_index[j] += dst_offset[j];
            }
            for(Index j = 0; j < tile_traits.nelems; ++j)
            {
                auto global_index = tile_traits.linear_to_index(j);
                for(Index k = 0; k < dst.ndim; ++k)
                {
                    global_index[k] += tile_index[k];
                }
                bool copied = true;
                for(Index k = 0; k < dst.ndim; ++k)
                {
                    if(global_index[k] < src_offset[k]
                            or global_index[k] >= src_offset[k]+src_shape[k])
                    {
                        copied = false;
                        break;
                    }
                }
                if(copied)
                {
                    TEST_ASSERT(tile_local_ptr[j]
                            == T(traits.index_to_linear(global_index)));
                }
                else
                {
                    TEST_ASSERT(tile_local_ptr[j] == T{-1});
                }
            }
            tile_local.release();
        }
    }
}

template<typename T>
void validate()
{
    check<T>({}, {}, {}, {}, {}, {}, {});
    check<T>({11, 12, 13}, {0, 0, 0}, {11, 12, 13}, {11, 12, 13}, {0, 0, 0},
            {11, 12, 13}, {11, 12, 13});
    check<T>({11, 12, 13}, {0, 0, 0}, {11, 12, 13}, {2, 3, 4}, {0, 0, 0},
            {11, 12, 13}, {2, 3, 4});
    check<T>({11, 12, 13}, {0, 0, 0}, {11, 12, 13}, {11, 12, 13}, {0, 0, 0},
            {11, 12, 13}, {2, 3, 4});
    check<T>({11, 12, 13}, {0, 0, 0}, {11, 12, 13}, {2, 3, 4}, {0, 0, 0},
            {11, 12, 13}, {11, 12, 13});
    check<T>({11, 12, 13}, {4, 3, 4}, {5, 5, 5}, {2, 3, 4}, {0, 0, 0},
            {11, 12, 13}, {2, 3, 4});
    check<T>({11, 12, 13}, {2, 0, 0}, {9, 12, 13}, {2, 3, 4}, {4, 3, 4},
            {5, 5, 5}, {2, 3, 4});
    check<T>({11, 12, 13}, {0, 0, 0}, {11, 12, 13}, {2, 3, 4}, {0, 0, 0},
            {11, 12, 13}, {3, 4, 5});
    check<T>({11, 12, 13}, {0, 0, 0}, {11, 12, 13}, {3, 4, 5}, {0, 0, 0},
            {11, 12, 13}, {2, 3, 4});
    check<T>({11, 12, 13}, {2, 2, 2}, {8, 8, 8}, {3, 4, 5}, {0, 0, 0},
            {11, 12, 13}, {2, 3, 4});
    check<T>({11, 12, 13}, {2, 2, 2}, {8, 8, 8}, {3, 4, 5}, {3, 3, 3},
            {8, 8, 8}, {2, 3, 4});
}

int main(int argc, char **argv)
{
    // Init StarPU for testing
    StarpuTest starpu;
    // Init codelet
    starpu::subcopy::init();
    // Launch all tests
    validate<fp32_t>();
    validate<fp64_t>();
    return 0;
}

