#include <cstddef>
#include <iterator>
#include <stdexcept>

#include <cuda.h>
#include <nvbench/launch.cuh>
#include <nvbench/nvbench.cuh>
#include <thrust/execution_policy.h>
#include <thrust/extrema.h>

#include "nntile/base_types.hh"
#include "nntile/kernel/maxsumexp.hh"
#include "nntile/kernel/maxsumexp/cuda.hh"

using nntile::Index;

namespace maxsumexp = nntile::kernel::maxsumexp;

enum class Device : int {
    kCPU = 0,
    kCUDA = 1,
};

template <typename T, Device device> struct Array;

template <typename T> struct Array<T, Device::kCUDA> {
    size_t size;
    T *data = nullptr;
    cudaError_t status = cudaSuccess;

    Array(size_t size) noexcept : size{size} {
        status = cudaMalloc(&data, size * sizeof(T));
    }

    ~Array(void) {
        if (data) {
            cudaFree(data);
            data = nullptr;
        }
    }

    operator bool(void) const {
        return status == cudaError_t::cudaSuccess;
    }

    template <typename U> U *as(void) {
        return reinterpret_cast<U *>(data);
    }

    template <typename U> U *as(void) const {
        return reinterpret_cast<U const *>(data);
    }
};

template <typename T>
__global__ void Copy(Index n, Index m, Index k, T const *src, T *dst) {
    auto ix = threadIdx.y + blockIdx.y * blockDim.y;
    auto jx = threadIdx.x + blockIdx.x * blockDim.x;
    auto kx = threadIdx.z + blockIdx.z * blockDim.z;
    if (ix >= n || kx >= m) {
        return;
    }
    for (auto jt = 0; jt != k; ++jt) {
        auto value = src[ix + n * (jx + jt) + n * m * kx];
        dst[ix + n * kx + 0] = value;
        dst[ix + n * kx + 1] = value;
    }
}

void BenchCopy(nvbench::state &state) {
    auto batch_size = static_cast<int>(state.get_int64("batch_size"));
    auto seq_len = static_cast<int>(state.get_int64("seq_len"));
    auto src = Array<float, Device::kCUDA>(batch_size * seq_len * seq_len);
    auto dst = Array<float, Device::kCUDA>(batch_size * seq_len * 2);

    // Request throughput stats.
    state.add_element_count(src.size);
    state.add_global_memory_reads<float>(src.size);
    state.add_global_memory_writes<float>(dst.size);
    state.exec(nvbench::exec_tag::sync, [&](nvbench::launch &launch) {
        dim3 threads(256);
        dim3 blocks(1, batch_size, seq_len);
        Copy<float><<<threads, blocks, 0, launch.get_stream()>>>(
            batch_size, seq_len, seq_len, src.as<float>(), dst.as<float>());
        cudaStreamSynchronize(launch.get_stream());
    });
}

NVBENCH_BENCH(BenchCopy)
    .add_int64_axis("batch_size", {2, 8, 32})
    .add_int64_axis("seq_len", {64, 256});

void BenchMaxSumExp(nvbench::state &state) {
    auto batch_size = static_cast<int>(state.get_int64("batch_size"));
    auto seq_len = static_cast<int>(state.get_int64("seq_len"));
    auto src = Array<float, Device::kCUDA>(batch_size * seq_len * seq_len);
    auto dst = Array<float, Device::kCUDA>(batch_size * seq_len * 2);

    // Request throughput stats.
    state.add_element_count(src.size);
    state.add_global_memory_reads<float>(src.size);
    state.add_global_memory_writes<float>(dst.size);
    state.exec(nvbench::exec_tag::sync, [&](nvbench::launch &launch) {
        maxsumexp::cuda(launch.get_stream(), batch_size, seq_len, seq_len,
                        src.as<float>(), dst.as<float>());
        cudaStreamSynchronize(launch.get_stream());
    });
}

NVBENCH_BENCH(BenchMaxSumExp)
    .add_int64_axis("batch_size", {2, 8, 32})
    .add_int64_axis("seq_len", {64, 256, 1024});

template <typename T, typename Distance = std::intptr_t, typename Pointer = T *,
          typename Reference = T &>
class StridedIterator : public std::iterator<std::random_access_iterator_tag,
                                             typename std::remove_cv<T>::type,
                                             Distance, Pointer, Reference> {
public:
    // TODO(@bershatsky): Use iterator traits.
    using difference_type = Distance;
    using pointer = Pointer;
    using reference = Reference;

public:
    T *ptr_;
    difference_type stride_;

public:
    __device__ StridedIterator(void) = delete;

    __device__ StridedIterator(StridedIterator const &that) noexcept
        : ptr_{that.ptr_}, stride_{that.stride_} {
    }

    __device__
    StridedIterator(T *ptr, difference_type stride = difference_type()) noexcept
        : ptr_{ptr}, stride_{stride} {
    }

    __device__ StridedIterator &operator++(void) {
        ptr_ += stride_;
        return *this;
    }

    __device__ StridedIterator operator++(int) {
        StridedIterator it(*this);
        ++(*this);
        return it;
    }

    __device__ StridedIterator &operator+=(difference_type offset) {
        ptr_ += offset * stride_;
        return *this;
    }

    __device__ StridedIterator operator+(difference_type offset) const {
        auto ptr = ptr_ + offset * stride_;
        return {ptr, stride_};
    }

    __device__ difference_type operator-(StridedIterator const &that) const {
        auto offset = ptr_ - that.ptr_;
        return offset / stride_;
    }

    __device__ bool operator==(StridedIterator const &that) const {
        return ptr_ == that.ptr_;
    }

    __device__ bool operator!=(StridedIterator const &that) const {
        return !(*this == that);
    }

    __device__ reference operator*(void) const {
        return *ptr_;
    }

    __device__ reference &operator*(void) {
        return *ptr_;
    }

    __device__ reference operator[](size_t index) const {
        return *(ptr_ + stride_ * index);
    }
};

template <typename T>
__global__ void MaxSumExp2(Index m, Index n, Index k, Index mk,
                           T const *__restrict__ src, T *__restrict__ dst) {
    auto ix = threadIdx.x + blockDim.x * blockIdx.x;
    auto kx = threadIdx.z + blockDim.z * blockIdx.z;
    if (ix >= m || kx >= n) {
        return;
    }

    StridedIterator begin(src + ix + mk * kx, m);
    StridedIterator end(src + ix + mk * (kx + 1), m);

    auto max = thrust::max_element(thrust::device, begin, end);
    auto unary = [x_max = *max](auto x) { return std::exp(x - x_max); };
    auto sum = thrust::transform_reduce(thrust::device, begin, end, unary, T(),
                                        thrust::plus<T>());

    dst = dst + ix + mk * kx;
    dst[0] = *max;
    dst[1] = sum;
}

using T = float;
void LaunchMaxSumExp2(cudaStream_t stream, Index m, Index n, Index k,
                      T const *src, T *dst) noexcept {
    dim3 threads(m, 1, n);
    dim3 blocks(1);
    MaxSumExp2<T><<<blocks, threads, 0, stream>>>(m, n, k, m * k, src, dst);
}

void BenchMaxSumExp2(nvbench::state &state) {
    auto batch_size = static_cast<int>(state.get_int64("batch_size"));
    auto seq_len = static_cast<int>(state.get_int64("seq_len"));
    auto src = Array<float, Device::kCUDA>(batch_size * seq_len * seq_len);
    auto dst = Array<float, Device::kCUDA>(batch_size * seq_len * 2);

    // Request throughput stats.
    state.add_element_count(src.size);
    state.add_global_memory_reads<float>(src.size);
    state.add_global_memory_writes<float>(dst.size);
    state.exec(nvbench::exec_tag::sync, [&](nvbench::launch &launch) {
        LaunchMaxSumExp2(launch.get_stream(), batch_size, seq_len, seq_len,
                         src.as<float>(), dst.as<float>());
        cudaStreamSynchronize(launch.get_stream());
    });
}

NVBENCH_BENCH(BenchMaxSumExp2)
    .add_int64_axis("batch_size", {2, 8, 32})
    .add_int64_axis("seq_len", {64, 256, 1024});

extern __shared__ float extent[]; // User-managed cache on device.

template <typename T>
__global__ void MaxSumExp3(Index m, Index n, Index k, Index mk,
                           T const *__restrict__ src, T *__restrict__ dst) {
    // Memory model of user-maneged cache in shared memory.
    size_t const data_size = blockDim.x * blockDim.y * blockDim.z;
    T *cache = reinterpret_cast<T *>(extent); // Mirror of global memory.
    // Accumulator for max-reduction and sum-reduction.
    T *acc = reinterpret_cast<T *>(cache) + data_size;

    auto ix = threadIdx.x + blockDim.x * blockIdx.x;
    auto jx = threadIdx.y + blockDim.y * blockIdx.y;
    auto kx = threadIdx.z + blockDim.z * blockIdx.z;
    if (ix >= m || jx >= k || kx >= n) {
        return;
    }

    // Load data from global memory to user-managed cache in shared memory.
    auto tid = threadIdx.x + blockDim.x * threadIdx.y +
               blockDim.x * blockDim.y * threadIdx.z; // Thread ID in block.
    cache[tid] = src[ix + m * jx + mk * kx];
    __syncthreads();

    // Per-block max-reduction in shared memory.
    auto fid = threadIdx.y; // Thread ID along fiber-axis (y).
    auto offset_local = threadIdx.x + blockDim.x * blockDim.y * threadIdx.z;
    acc[tid] = cache[tid]; // TODO: max + 2 x load from cache.
    __syncthreads();
    for (auto stride = 1; stride < blockDim.y; stride *= 2) {
        if (fid % (2 * stride) == 0) {
            acc[offset_local + fid] =
                max(acc[offset_local + blockDim.x * fid],
                    acc[offset_local + blockDim.x * (fid + stride)]);
        }
        __syncthreads();
    }

    // Per-block sumexp-reduction in shared memory.
    auto max = acc[offset_local];
    acc[tid] = std::exp(cache[tid] - max); // TODO: The same.
    __syncthreads();
    for (auto stride = 1; stride < blockDim.y; stride *= 2) {
        if (fid % (2 * stride) == 0) {
            acc[offset_local + fid] +=
                acc[offset_local + blockDim.x * (fid + stride)];
        }
        __syncthreads();
    }

    // Store in global memory (output buffer) in theads from X-Z plane.
    if (fid == 0) {
        // Contingues tuple of (max, sum). Update accumulants in-place.
        auto out = dst + 2 * (ix + m * kx);
        if (auto diff = max - out[0]; diff > 0) {
            out[0] = max;
            out[1] = out[1] * std::exp(-diff) + acc[offset_local];
        } else {
            out[1] = out[1] + std::exp(diff) * acc[offset_local];
        }
    }
}

template <typename T>
void LaunchMaxSumExp3(cudaStream_t stream, Index m, Index n, Index k,
                      T const *src, T *dst) noexcept {
    dim3 threads(1, 1024, 1);
    dim3 blocks(m, 1, n);
    size_t smem = 2 * threads.x * threads.y * threads.z * sizeof(T);
    MaxSumExp3<T><<<blocks, threads, smem, stream>>>(m, n, k, m * k, src, dst);
}

void BenchMaxSumExp3(nvbench::state &state) {
    auto batch_size = static_cast<int>(state.get_int64("batch_size"));
    auto seq_len = static_cast<int>(state.get_int64("seq_len"));
    auto src = Array<float, Device::kCUDA>(batch_size * seq_len * seq_len);
    auto dst = Array<float, Device::kCUDA>(batch_size * seq_len * 2);

    // Request throughput stats.
    state.add_element_count(src.size);
    state.add_global_memory_reads<float>(src.size);
    state.add_global_memory_writes<float>(dst.size);
    state.exec(nvbench::exec_tag::sync, [&](nvbench::launch &launch) {
        LaunchMaxSumExp3(launch.get_stream(), batch_size, seq_len, seq_len,
                         src.as<float>(), dst.as<float>());
        // cudaStreamSynchronize(launch.get_stream());
        cudaDeviceSynchronize();
        cudaError_t status = cudaGetLastError();
        if (status != cudaSuccess) {
            std::cout << status << " " << cudaGetErrorName(status) << " "
                      << cudaGetErrorString(status) << std::endl;
            throw std::runtime_error("CUDA error");
        }
    });
}

NVBENCH_BENCH(BenchMaxSumExp3)
    .add_int64_axis("batch_size", {2, 8, 32})
    .add_int64_axis("seq_len", {64, 256, 1024});
