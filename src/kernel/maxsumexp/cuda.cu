/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/kernel/maxsumexp/cuda.cu
 * Max and sum of exponents of a buffer on CUDA
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-07-06
 * */

#include "nntile/kernel/maxsumexp/cuda.hh"

namespace nntile
{
namespace kernel
{
namespace maxsumexp
{

template<typename T>
static __global__
void cuda_kernel(Index m, Index n, Index k, Index mk,
        const T * __restrict__ src, T * __restrict__ maxsumexp)
{
    Index i1 = threadIdx.y + blockIdx.y*blockDim.y,
          i2 = threadIdx.z + blockIdx.z*blockDim.z,
          i0_start = threadIdx.x, i0_step = blockDim.x;
    constexpr T zero = 0.0, one = 1.0;
    if(i2 < n and i1 < m and i0_start < k)
    {
        // Get max and sum of exponents of a corresponding slice
        const T *src_slice = src + i2*mk + i1;
        // Init max and sum
        T max_val = src_slice[i0_start*m];
        T sum_val = one;
        // Cycle over slice of input buffer
        for(Index i0 = i0_start+i0_step; i0 < k; i0 += i0_step)
        {
            // Read value from source
            T val = src_slice[i0*m];
            // Ignore -inf value, which comes from mask
            if(::isinf(val))
            {
                continue;
            }
            // Update max and sum of exponents
            if(max_val < val)
            {
                sum_val = sum_val*(::exp(max_val-val)) + one;
                max_val = val;
            }
            else
            {
                sum_val += ::exp(val-max_val);
            }
        }
        // Per-block of threads max and sum of exponents
        volatile __shared__ T block_max_val;
        __shared__ T block_sum_val;
        // Accumulate result into a global sum. Different branches for
        // i0_start==0 thread and others
        if(i0_start > 0)
        {
            // Finish early if max_val is -inf (masked out)
            if(::isinf(max_val))
            {
                return;
            }
            // Wait until i0_start==0 threads initializes block_max_val
            __synthreads();
            // Update max at first, max_val is finite here
            while(block_max_val < max_val)
            {
                block_max_val = max_val;
            }
            // Sync with all other threads, including i0_start==0, to get
            // per-block maximum value
            __syncthreads();
            // This thread has a finite max_val and sum_val, so no NaN shall
            // appear here
            sum_val *= ::exp(max_val - block_max_val);
            // Accumulate into global sum
            atomicAdd(&block_sum_val, sum_val);
            __syncthreads();
        }
        else
        {
            block_max_val = max_val;
            block_sum_val = zero;
            // Other threads wait until shared data is initialised
            __syncthreads();
            // Wait for other alive threads to acquire per-block max value
            __syncthreads();
            // Finally, wait other threads to accumulate sum of exponents
            __syncthreads();
            // Do not update accumulated result if local max is -inf
            if(not ::isinf(max_val))
            {
                block_sum_val += sum_val * ::exp(max_val-block_max_val);
            }
            // Get per-block max value and sum of exponents, finally
            max_val = block_max_val;
            sum_val = block_sum_val;
            Index dst_offset = i1 + i2*m;
            // Special case of max_val equal -inf (all elements are masked out)
            // Do not update data in global memory in this case
            if(::isinf(max_val))
            {
                return;
            }
            // Now max_val is finite, we need to accumulate sum of exponents
            // with the data in global memory
            T max_output = maxsumexp[2*dst_offset];
            T sum_output = maxsumexp[2*dst_offset+1];
            // If data was not yet initialised, just overwrite it
            if(sum_output == zero)
            {
                max_output = max_val;
                sum_output = sum_val;
            }
            // Accumulate otherwise
            else
            {
                if(max_val < max_output)
                {
                    sum_val *= ::exp(max_val-max_output);
                }
                else
                {
                    sum_output *= ::exp(max_output-max_val);
                    max_output = max_val;
                }
                sum_output += sum_val;
            }
            maxsumexp[2*dst_offset] = max_output;
            maxsumexp[2*dst_offset+1] = sum_output;
        }
    }
}

template<typename T>
void cuda(cudaStream_t stream, Index m, Index n, Index k, const T *src,
        T *maxsumexp)
    noexcept
//! Max and sum of exponents along middle axis
/*! For a provided m-by-k-by-n input array src compute maximums and sums of
 * exponents of slices along second axis with k elements, resulting in
 * 2-by-m-by-n output array maxsumexp.
 *
 *      old[0,i,j] = maxsumexp[0,i,j]
 *      old[1,i,j] = maxsumexp[1,i,j]
 *      maxsumexp[0,i,j] = max(old[0,i,j], max(src[i,:,j]))
 *      maxsumexp[1,i,j] = old[1,i,j]*exp(old[0,i,j]-maxsumexp[0,i,j])
 *          + sum(exp(src[i,:,j]-maxsumexp[0,i,j])))
 *
 * @param[in] m: Size of the first mode of src and the second mode of maxsumexp
 *      arrays.
 * @param[in] n: Size of the last mode of src and maxsumexp arrays
 * @param[in] k: Size of the middle mode of src array
 * @param[in] src: Input contiguous m-by-k-by-n array
 * @param[inout] maxsumexp: Output contiguous 2-by-m-by-n array, that accumulates
 *      sums and norms of slices along middle axis.
 * */
{
    // Source is an m-by-n matrix and destination is an m-by-k-by-n tensor
    // Both source and destination are Fortran-contiguous
    dim3 threads(256, 1, 1);
    dim3 blocks(1, m, n);
    (cuda_kernel<T>)<<<blocks, threads, 0, stream>>>(m, n, k, m*k, src,
            maxsumexp);
}

// Explicit instantiation
template
void cuda<fp32_t>(cudaStream_t stream, Index m, Index n, Index k,
        const fp32_t *src, fp32_t *maxsumexp)
    noexcept;

template
void cuda<fp64_t>(cudaStream_t stream, Index m, Index n, Index k,
        const fp64_t *src, fp64_t *maxsumexp)
    noexcept;

} // namespace maxsumexp
} // namespace kernel
} // namespace nntile

