/*! @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
 *                           (Skoltech). All rights reserved.
 *
 * NNTile is software framework for fast training of big neural networks on
 * distributed-memory heterogeneous systems based on StarPU runtime system.
 *
 * @file tests/kernel/addcdiv.cc
 * Per-element addcdiv operation for buffers
 *
 * @version 1.0.0
 * @author Aleksandr Katrutsa
 * @date 2023-02-14
 * */

#include "nntile/kernel/addcdiv.hh"
#include "../testing.hh"
#include <vector>
#include <stdexcept>
#include <cmath>
#include <iostream>

using namespace nntile;
using namespace nntile::kernel::addcdiv;

#ifdef NNTILE_USE_CUDA
template<typename T>
void run_cuda(T val, T eps, Index nelems, T* nom, T* denom, T* src)
{
    // Copy to device
    T *dev_src, *dev_nom, *dev_denom;
    cudaError_t cuda_err = cudaMalloc(&dev_src, sizeof(T)*nelems);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMalloc(&dev_nom, sizeof(T)*nelems);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMalloc(&dev_denom, sizeof(T)*nelems);
    TEST_ASSERT(cuda_err == cudaSuccess);

    cuda_err = cudaMemcpy(dev_src, &src[0], sizeof(T)*nelems,
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_denom, &denom[0], sizeof(T)*nelems,
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaMemcpy(dev_nom, &nom[0], sizeof(T)*nelems,
            cudaMemcpyHostToDevice);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Init stream
    cudaStream_t stream;
    cuda_err = cudaStreamCreate(&stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Launch low-level CUDA kernel
    cuda<T>(stream, val, eps, nelems, dev_nom, dev_denom, dev_src);
    cuda_err = cudaStreamSynchronize(stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
    // Copy result and deallocate device memory
    cuda_err = cudaMemcpy(&src[0], dev_src, sizeof(T)*nelems,
            cudaMemcpyDeviceToHost);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_src);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_nom);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaFree(dev_denom);
    TEST_ASSERT(cuda_err == cudaSuccess);
    cuda_err = cudaStreamDestroy(stream);
    TEST_ASSERT(cuda_err == cudaSuccess);
}
#endif // NNTILE_USE_CUDA

// Templated validation
template<typename T>
void validate(T val, T eps, Index nelems)
{
    // Init test input
    std::vector<T> src(nelems), nom(nelems), denom(nelems);
    T sign_factor = T(-1.);
    for(Index i = 0; i < nelems; ++i)
    {
        src[i] = T(2*i+1-nelems) * sign_factor;
        nom[i] = T(nelems-i);
        denom[i] = T(i+1);
        sign_factor *= T(-1.);
    }
    std::vector<T> src_copy(src);
    // Check low-level CPU kernel
    std::cout << "Run kernel::addcdiv::cpu<T>\n";
    cpu<T>(val, eps, nelems, &nom[0], &denom[0], &src[0]);
    T ref_val;
    for(Index i = 0; i < nelems; ++i)
    {
        TEST_ASSERT(src[i] == (src_copy[i] + val * nom[i] / (denom[i] + eps)));
    }
    std::cout << "OK: kernel::addcdiv::cpu<T>\n";
#ifdef NNTILE_USE_CUDA
    // Check low-level CUDA kernel
    src = src_copy;
    std::cout << "Run kernel::addcdiv::cuda<T>\n";
    run_cuda<T>(val, eps, nelems, &nom[0], &denom[0], &src[0]);
    for(Index i = 0; i < nelems; ++i)
    {
        TEST_ASSERT(src[i] == (src_copy[i] + val * nom[i] / (denom[i] + eps)));
    }
    std::cout << "OK: kernel::addcdiv::cuda<T>\n";
#endif // NNTILE_USE_CUDA
}

int main(int argc, char **argv)
{
    validate<fp32_t>(0, 0, 0);
    validate<fp32_t>(1, 1e-5, 10);
    validate<fp32_t>(-5, 1e-2, 80000);

    validate<fp64_t>(0, 0, 0);
    validate<fp64_t>(1, 1e-5, 10);
    validate<fp64_t>(-5, 1e-2, 80000);
    return 0;
}
