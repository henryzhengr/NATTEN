/***************************************************************************************************
 * Copyright (c) 2023 Ali Hassani.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 **************************************************************************************************/
/*! \file
    \brief Inverse-Neighborhood-Neighborhood CPU kernel for 2D data.
           Applies inverse neighborhood attention weights to inverse neighborhood values.
           Used to compute key and value grads.
*/

#include <torch/extension.h>
#include <vector>
#include <ATen/ATen.h>
#include <ATen/AccumulateType.h>

#if defined(AVX_INT)
#include <ATen/cpu/vec/functional.h>
#include <ATen/cpu/vec/vec.h>
#endif

#include "natten_cpu_commons.h"

namespace natten {

template<class scalar_t>
using Tensor5D = typename at::TensorAccessor<scalar_t, 5>;

#define GRAIN_SIZE 0

// TODO: AVX

template <int KS, int NS, int DILATION, typename scalar_t>
void inverse_neighborhood_2d(          // K-grad / V-grad
    const Tensor5D<scalar_t> weights,  // d_attn / attn
    const Tensor5D<scalar_t> values,   // query  / d_out
    Tensor5D<scalar_t> output,         // d_key  / d_value
    const int height, 
    const int width,
    const int heads,
    const int kernel_size_in,
    const int dilation_in,
    const int dim,
    const int batch_size) {
    const int KERNEL_SIZE = (KS>1) ? KS : kernel_size_in;
    const int NEIGHBORHOOD_SIZE = (NS>0) ? NS : KERNEL_SIZE / 2;
    const int dilation = (DILATION>0) ? DILATION : dilation_in;
    for (int b = 0; b < batch_size; b++) {
        at::parallel_for(0, heads, GRAIN_SIZE, [&](int start, int end) {
        for (int h = start; h < end; h++) {
            for (int i = 0; i < height; i++) {
            const int ni = get_backward_window_start(i, KERNEL_SIZE, NEIGHBORHOOD_SIZE, dilation);
            const int ei = get_backward_window_end(i, height, KERNEL_SIZE, NEIGHBORHOOD_SIZE, dilation);
            for (int j = 0; j < width; j++) {
                const int nj = get_backward_window_start(j, KERNEL_SIZE, NEIGHBORHOOD_SIZE, dilation);
                const int ej = get_backward_window_end(j, width, KERNEL_SIZE, NEIGHBORHOOD_SIZE, dilation);
                for (int d = 0; d < dim; d++) {
                    const int weightsOffset = b * weights.stride(0) + h * weights.stride(1);
                    const int outOffset = b * values.stride(0) + h * values.stride(1) + d;
                    scalar_t output_update = scalar_t(0);
                    for (int xi=ni; xi < ei; xi+=dilation){
                    const int oni = get_window_start(xi, height, KERNEL_SIZE, NEIGHBORHOOD_SIZE, dilation);
                    for (int xj=nj; xj < ej; xj+=dilation){
                        const int onj = get_window_start(xj, width, KERNEL_SIZE, NEIGHBORHOOD_SIZE, dilation);
                        const int outIndex = outOffset + xi * values.stride(2) + xj * values.stride(3);
                        const int weightsIndex = weightsOffset + xi * weights.stride(2) + xj * weights.stride(3) + int((i-oni)/dilation)*KERNEL_SIZE + int((j-onj)/dilation);
                        output_update += values.data()[outIndex] * weights.data()[weightsIndex];
                    }}
                    const int linearIndex = b*output.stride(0) + h*output.stride(1) + i*output.stride(2) + j*output.stride(3) + d;
                    output.data()[linearIndex] = output_update;
                }
            }}
        }});
    }
}

} // namespace natten
