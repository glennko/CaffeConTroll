
#ifndef _KERNEL_POOL_HXX
#define _KERNEL_POOL_HXX

#include "pool.h"

#ifdef _GPU_TARGET
__host__ __device__
#endif
inline size_t _f_src_to_dst_pool_forward(size_t src_pos, void * const _arg) {
  const _pool_forward_arg_helper * const arg = (_pool_forward_arg_helper *) _arg;
  const int iR = arg->iR;
  const int iC = arg->iC;
  const int oR = arg->oR;
  const int oC = arg->oC;
  return (src_pos/iR/iC)*oR*oC;
}

#ifdef _GPU_TARGET
__host__ __device__
#endif
inline void _f_pool_forward(void * output, void * input, void * const _arg,
    const size_t dst_index) {
  const _pool_forward_arg_helper * const arg = (_pool_forward_arg_helper *) _arg;
  const int oR = arg->oR;
  const int oC = arg->oC;
  const int stride = arg->stride;
  const int kernel_size = arg->kernel_size;
  const int iR = arg->iR;
  const int iC = arg->iC;
  size_t * const max_index = (size_t *) (&arg->max_index[dst_index / sizeof(float) * sizeof(size_t)]);
  float * const input_data = (float *) input;
  float * const output_data = (float *) output;

  for (int ph = 0; ph < oR; ++ph) {
    const int h_start = ph * stride;
    const int h_end = min(h_start + kernel_size, iR);
    for (int pw = 0; pw < oC; ++pw) {
      const int w_start = pw * stride;
      const int w_end = min(w_start + kernel_size, iC);
      const int pool_index = ph * oC + pw;
      for (int h = h_start; h < h_end; ++h) {
        for (int w = w_start; w < w_end; ++w) {
          const int index = h * iC + w;
          max_index[pool_index] = input_data[index] > output_data[pool_index] ?
            index : max_index[pool_index];
          output_data[pool_index] = input_data[index] > output_data[pool_index] ?
            input_data[index] : output_data[pool_index];
        }
      }
    }
  }
}

#ifdef _GPU_TARGET
__host__ __device__
#endif
inline size_t _f_src_to_dst_pool_backward(size_t src_pos, void * const _arg) {
  const _pool_backward_arg_helper * const arg = (_pool_backward_arg_helper *) _arg;
  const int iR = arg->iR;
  const int iC = arg->iC;
  const int oR = arg->oR;
  const int oC = arg->oC;
  return (src_pos/iR/iC)*oR*oC;
}

#ifdef _GPU_TARGET
__host__ __device__
#endif
inline void _f_pool_backward(void * output, void * input, void * const _arg,
    const size_t dst_index) {
  const _pool_backward_arg_helper * const arg = (_pool_backward_arg_helper *) _arg;
  const int oR = arg->oR;
  const int oC = arg->oC;
  size_t * const max_index = (size_t *) (&arg->max_index[dst_index / sizeof(float) * sizeof(size_t)]);
  float * const input_grad = (float *) input;
  float * const output_grad = (float *) output;

  for (int ph = 0; ph < oR; ++ph) {
    for (int pw = 0; pw < oC; ++pw) {
      const size_t index = ph * oC + pw;
      const size_t input_grad_index = max_index[index];
      input_grad[input_grad_index] += output_grad[index];
    }
  }
}

#endif
