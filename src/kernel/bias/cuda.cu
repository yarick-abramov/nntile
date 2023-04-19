/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/kernel/bias/cuda.cu
 * Bias-like product operation on a buffer on CUDA
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-04-19
 * */

#include "nntile/kernel/biasprod/cuda.hh"

namespace nntile
{
namespace kernel
{
namespace biasprod
{

template<typename T>
static __global__
void cuda_kernel(Index m, Index n, Index k, Index mk, T alpha, const T *src,
        T *dst)
{
    Index i2_start = threadIdx.x + blockIdx.x*blockDim.x,
          i1_start = threadIdx.y + blockIdx.y*blockDim.y,
          i2_step = blockDim.x * gridDim.x,
          i1_step = blockDim.y * gridDim.y;
    for(Index i2 = i2_start; i2 < n; i2 += i2_step)
    {
        for(Index i1 = i1_start; i1 < m; i1 += i1_step)
        {
            T *dst_slice = dst + i2*mk + i1;
            const T src_val = alpha * src[i2*m+i1];
            for(Index i0 = 0; i0 < k; ++i0)
            {
                // Read value from source
                T &dst_val = dst_slice[i0*m];
                dst_val = dst_val * src_val;
            }
        }
    }
}

template<typename T>
void cuda(cudaStream_t stream, Index m, Index n, Index k, T alpha,
        const T *src, T *dst)
    noexcept
//! Bias-like product along middle axis on CUDA
/*! For a provided m-by-k-by-n output tensor dst apply bias-like product along
 * second axis with k elements from m-by-n input tensor src:
 *      dst[i, l, j] = dst[i, l, j] * src[i, j]
 *
 * It is possible API will be extended some day to the following:
 *      dst[i, l, j] = dst[i, l, j]^beta * src[i, j]^alpha
 *
 * @param[in] m: Size of the first mode of src and dst tensors
 * @param[in] n: Size of the last mode of src and dst tensors
 * @param[in] k: Size of the middle mode of dst tensor
 * @param[in] src: Source of the bias
 * @param[inout] dst: Destination of the bias
 * */
{
    // Source is an m-by-n matrix and destination is an m-by-k-by-n tensor
    // Both source and destination are Fortran-contiguous
    dim3 blocks(16, 16), threads(8, 4);
    (cuda_kernel<T>)<<<blocks, threads, 0, stream>>>(m, n, k, m*k, src, dst);
}

// Explicit instantiation
template
void cuda<fp32_t>(cudaStream_t stream, Index m, Index n, Index k,
        const fp32_t *src, fp32_t *dst)
    noexcept;

template
void cuda<fp64_t>(cudaStream_t stream, Index m, Index n, Index k,
        const fp64_t *src, fp64_t *dst)
    noexcept;

} // namespace biasprod
} // namespace kernel
} // namespace nntile

