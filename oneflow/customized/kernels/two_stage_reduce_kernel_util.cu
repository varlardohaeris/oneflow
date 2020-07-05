#include "oneflow/customized/kernels/two_stage_reduce_kernel_util.h"

namespace oneflow {

namespace {

template<typename T, typename K>
__global__ void DivideGpu(const int64_t n, const T* x, const K* count, T* y) {
  CUDA_1D_KERNEL_LOOP(i, n) { y[i] = x[i] / count[i]; }
}

template<typename T, typename K>
__global__ void MaskGpu(const int64_t n, const T* x, const K* mask, T* y) {
  CUDA_1D_KERNEL_LOOP(i, n) { y[i] = static_cast<T>(mask[i]) * x[i]; }
}

template<typename T, typename K>
__global__ void ScaleGpu(const int64_t n, const T* x, const K* scale, T* y) {
  CUDA_1D_KERNEL_LOOP(i, n) { y[i] = x[i] * scale[i]; }
}

}  // namespace

template<typename T, typename K>
struct TwoStageReduceKernelUtil<DeviceType::kGPU, T, K> {
  static void Divide(DeviceCtx* ctx, const int64_t n, const T* x, const K* count, T* y) {
    DivideGpu<T, K><<<BlocksNum4ThreadsNum(n), kCudaThreadsNumPerBlock, 0, ctx->cuda_stream()>>>(
        n, x, count, y);
  }

  static void Mask(DeviceCtx* ctx, const int64_t n, const T* x, const K* mask, T* y) {
    MaskGpu<T, K><<<BlocksNum4ThreadsNum(n), kCudaThreadsNumPerBlock, 0, ctx->cuda_stream()>>>(
        n, x, mask, y);
  }

  static void Scale(DeviceCtx* ctx, const int64_t n, const T* x, const K* scale, T* y) {
    ScaleGpu<T, K><<<BlocksNum4ThreadsNum(n), kCudaThreadsNumPerBlock, 0, ctx->cuda_stream()>>>(
        n, x, scale, y);
  }
};

#define INSTANTIATE_TWO_STAGE_REDUCE_KERNEL_UTIL_GPU(data_type_pair, index_type_pair)          \
  template struct TwoStageReduceKernelUtil<DeviceType::kGPU, OF_PP_PAIR_FIRST(data_type_pair), \
                                           OF_PP_PAIR_FIRST(index_type_pair)>;
OF_PP_SEQ_PRODUCT_FOR_EACH_TUPLE(INSTANTIATE_TWO_STAGE_REDUCE_KERNEL_UTIL_GPU,
                                 FLOATING_DATA_TYPE_SEQ INDEX_DATA_TYPE_SEQ, INT_DATA_TYPE_SEQ);
#undef INSTANTIATE_TWO_STAGE_REDUCE_KERNEL_UTIL_GPU

}  // namespace oneflow
