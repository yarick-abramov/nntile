# @copyright (c) 2022-2023 Skolkovo Institute of Science and Technology
#                           (Skoltech). All rights reserved.
#
# NNTile is software framework for fast training of big neural networks on
# distributed-memory heterogeneous systems based on StarPU runtime system.
#
# @file wrappers/python/tests/nntile_core/test_tensor_sumnorm.py
# Test for tensor::sumnorm<T> Python wrapper
#
# @version 1.0.0
# @author Aleksandr Mikhalev
# @date 2023-02-09

# All necesary imports
import nntile
import numpy as np
# Set up StarPU configuration and init it
config = nntile.starpu.Config(1, 0, 0)
# Init all NNTile-StarPU codelets
nntile.starpu.init()
# Define list of tested types
dtypes = [np.float32, np.float64]
# Define mapping between numpy and nntile types
Tensor = {np.float32: nntile.tensor.Tensor_fp32,
        np.float64: nntile.tensor.Tensor_fp64}
# Define mapping between tested function and numpy type
sumnorm = {np.float32: nntile.nntile_core.tensor.sumnorm_fp32,
        np.float64: nntile.nntile_core.tensor.sumnorm_fp64}

# Helper function returns bool value true if test passes
def helper(dtype):
    # Describe single-tile tensor, located at node 0
    A_shape = [2, 3, 4]
    B_shape = []
    ndim = len(A_shape)
    for i in range(ndim):
        B_shape.append([2]+A_shape[:i]+A_shape[i+1:])
    mpi_distr = [0]
    next_tag = 0
    A_traits = nntile.tensor.TensorTraits(A_shape, A_shape)
    B_traits = []
    for i in range(ndim):
        B_traits.append(nntile.tensor.TensorTraits(B_shape[i], B_shape[i]))
    # Tensor objects
    A = Tensor[dtype](A_traits, mpi_distr, next_tag)
    next_tag = A.next_tag
    B = []
    for i in range(ndim):
        B.append(Tensor[dtype](B_traits[i], mpi_distr, next_tag))
        next_tag = B[-1].next_tag
    # Set initial values of tensors
    rand_A = np.random.randn(*A_shape)
    np_A = np.array(rand_A, dtype=dtype, order='F')
    A.from_array(np_A)
    np_B = []
    for i in range(ndim):
        np_B.append(np.zeros(B_shape[i], dtype=dtype, order='F'))
        B[i].from_array(np_B[-1])
    # Check result along each axis
    for i in range(ndim):
        sumnorm[dtype](A, B[i], i)
        B[i].to_array(np_B[i])
        nntile.starpu.wait_for_all()
        B[i].unregister()
        np_C = np.array([np.sum(np_A, axis=i), np.sum(np_A**2, axis=i)**0.5])
        if not np.allclose(np_B[i], np_C):
            return False
    A.unregister()
    return True

# Test runner for different precisions
def test():
    for dtype in dtypes:
        assert helper(dtype)

# Repeat tests
def test_repeat():
    for dtype in dtypes:
        assert helper(dtype)

if __name__ == "__main__":
    test()
    test_repeat()

