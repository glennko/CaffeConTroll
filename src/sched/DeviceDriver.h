
#include <functional>
#include <math.h>
#include <random>
#include "DeviceHeader.h"
#include "../kernels/lowering.h"

#ifndef _DEVICE_DRIVER_H
#define _DEVICE_DRIVER_H

using std::max;
using std::min;
using std::random_device;
using std::mt19937;
using std::uniform_real_distribution;
using std::bernoulli_distribution;
using std::normal_distribution;

/**
 * A DeviceDriver is the only way
 * that CcT talks to a certain device
 * to invoke computation and data
 * movement *INSIDE* a device. 
 *
 * Given a DeviceDriver, all Bridges
 * should be purely *logical*. This is
 * the goal of introducing DeviceDriver.
 *
 * All cross device operation needs to
 * be done via derefercing a DeviceMemoryPointer,
 * it is not DeviceDriver's job to worry
 * about cross-device data movement.
 *
 * A DeviceDriver needs to provide certain
 * interface, e.g., how BLAS can be called,
 * or different helper functions, e.g.,
 * axpy.
 * 
 * One question is what function should we
 * put in DeviceDriver and what function should
 * we put in Util? The answer is that
 * Util contains all functions that are
 * device-independent, e.g., get_learning_rate,
 * and DeviceDriver contains all functions
 * that are device-dependent. 
 *
 * Error handelling. All functions return void, however
 * if error occurs, it assert(false). In short, we
 * assume it is the worker's repsonbility to deal
 * with error, instead of the caller.
 *
 * TODO:
 *  - Template this by double, float etc.
 **/
class DeviceDriver{
public:

  /**
   * A UDF that can be called by a driver might sit on
   * host or might sit on device. It is driver's responsiblity
   * to choose one.
   *
   * Note that, a single function should have ONLY ONE 
   * implementation. It is on a device not by copy&paste
   * the code, instead, by a very thin wrapper, e.g., for CUDA,
   *     __device__ T func_on_device = func_on_host;
   **/
  virtual void * choose_ptr(void * host, void * device) = 0;

  virtual DeviceMemoryPointer * get_device_pointer(void * ptr, size_t size_in_byte) = 0;

  /**
   * Memory manipulation and data movement.
   **/
  virtual void malloc(DeviceMemoryPointer * dst) = 0;
  virtual void free(DeviceMemoryPointer * dst) = 0;
  virtual void memcpy(DeviceMemoryPointer * dst, DeviceMemoryPointer * src) = 0;
  virtual void memset(DeviceMemoryPointer * dst, const char value) = 0;

  template<FPMAP_ID f_id, FPMAP_DATA_READC f_data>
  void pmap2d_read_coalesce(DeviceMemoryPointer * dst, DeviceMemoryPointer * src, 
    const struct PMapHelper args);

  //virtual void pmap2d_write_coalesce();

  /**
   * This function implements the following semantic.
   *   for(i=0;i<src.size;i+=src_skip)
   *      func(&dst[f_dst_pos(j)], &src[i])
   * As the name implied, this might be run in parallel.
   *
   * For CPU Device, this could be a simple OpenMP parallel loop.
   * For GPU Device, this could be a kernel that uses func.
   *
   * Strictly speaking, this function has the name `map` mainly because
   * is access pattern and ability to be executed in parallel, instead of
   * its semantic--It is possible to introduce side-effect on `src` and 
   * `f_dst_pos` is not necessarily non-overlapping (it often is, but the 
   * interface does not have a way to enforce it). So maybe a better
   * way of thinking about this function is a parallel for loop with more 
   * structured side-effect (i.e., only on src and dst with a known mapping).
   * 
   **/
  template<FUNC_IDX_MAPPING f_dst_pos, FUNC_MM_MAPPING func>
  void parallel_map(DeviceMemoryPointer * dst, DeviceMemoryPointer * src, 
    size_t src_skip, DeviceMemoryPointer * const f_dst_pos_curry, 
    DeviceMemoryPointer * const func_curry) ;

  /**
   * Single-precision operations.
   **/
  virtual void smath_axpy(const float alpha, DeviceMemoryPointer * X, DeviceMemoryPointer * Y) = 0;

  template<FUNC_STRANSFORM func>
  void sapply(DeviceMemoryPointer * dst, DeviceMemoryPointer * const func_curry) ;

  virtual void smath_axpby(const float alpha, DeviceMemoryPointer * X, const float beta, DeviceMemoryPointer *Y) = 0;
  virtual void set_num_threads(const int nThreads) = 0;
  virtual void sgemm(const enum CBLAS_ORDER order, CBLAS_TRANSPOSE TA, CBLAS_TRANSPOSE TB, 
        int M, int N, int K, float alpha, float * pA, int LDA, float * pB, int LDB,
        float beta, float * pC, int LDC) = 0;

  template<FUNC_SREDUCE func>
  void selementwise_reduce2(DeviceMemoryPointer *dst, DeviceMemoryPointer *src1, 
    DeviceMemoryPointer *src2, DeviceMemoryPointer * const func_curry) ;

  virtual void sinitialize_xavier(DeviceMemoryPointer *arr, const size_t n_batch){
    assert(false);
  }

  virtual void sbernoulli_initialize(DeviceMemoryPointer *arr, const float p){
    assert(false);
  }

  virtual void sgaussian_initialize(DeviceMemoryPointer *arr, const float mean, const float std_dev){
    assert(false);
  }

  virtual void sconstant_initialize(DeviceMemoryPointer *arr, const float value){
    assert(false);
  }
    
  void smath_apply_grad(DeviceMemoryPointer *X, DeviceMemoryPointer *Y) {
    smath_axpy(-1.0, Y, X);
  }

};

#endif
