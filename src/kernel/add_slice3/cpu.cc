/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file src/kernel/add_slice3/cpu.cc
 * Per-element addition of a tensor and a broadcasted slice on CPU
 *
 * @version 1.0.0
 * @author Aleksandr Mikhalev
 * @date 2023-07-03
 * */

#include "nntile/kernel/add_slice3/cpu.hh"

namespace nntile
{
namespace kernel
{
namespace add_slice3
{

template<typename T>
void cpu(Index m, Index n, Index k, T alpha, const T *src1, T beta,
        const T *src2, T *dst)
    noexcept
//! Per-element addition of a tensor and a broadcasted slice on CPU
/*! Performs the following operations:
 *      dst[i,l,j] = alpha*src1[i,j] + beta*src2[i,l,j]
 *
 * @param[in] m: Size of the first mode of src1, src2 and dst tensors
 * @param[in] n: Size of the last mode of src1, src2 and dst tensors
 * @param[in] k: Size of the middle mode of src2 and dst tensor
 * @param[in] alpha: Scalar factor for src1
 * @param[in] src1: Input contiguous m-by-n array
 * @param[in] beta: Scaling factor for src1
 * @param[in] src2: Input contiguous m-by-k-by-n array
 * @param[out] dst: Output contiguous m-by-k-by-n array
 * */
{
    const Index mk = m * k;
    constexpr T zero = 0.0;
    // Cycle over column of the output buffer dst
    for(Index i2 = 0; i2 < n; ++i2)
    {
        // Cycle over row of the output buffer dst
        for(Index i1 = 0; i1 < m; ++i1)
        {
            // Pointer to a corresponding fiber of the input array src2
            const T *src2_fiber = src2 + i2*mk + i1;
            // Pointer to a corresponding fiber of the output array dst
            T *dst_fiber = dst + i2*mk + i1;
            // Value to add to the output fiber
            const T src1_val = alpha * src1[i2*m+i1];
            // Overwrite or update output depending on beta
            if(beta == zero)
            {
                // Cycle over output fiber elements
                for(Index i0 = 0; i0 < k; ++i0)
                {
                    // Set output value
                    dst_fiber[i0*m] = src1_val;
                }
            }
            else
            {
                // Cycle over output fiber elements
                for(Index i0 = 0; i0 < k; ++i0)
                {
                    // And update it
                    dst_fiber[i0*m] = beta*src2_fiber[i0*m] + src1_val;
                }
            }
        }
    }
}

// Explicit instantiation
template
void cpu<fp32_t>(Index m, Index n, Index k, fp32_t alpha, const fp32_t *src1,
        fp32_t beta, const fp32_t *src2, fp32_t *dst)
    noexcept;

template
void cpu<fp64_t>(Index m, Index n, Index k, fp64_t alpha, const fp64_t *src1,
        fp64_t beta, const fp64_t *src2, fp64_t *dst)
    noexcept;

} // namespace add_slice3
} // namespace kernel
} // namespace nntile

