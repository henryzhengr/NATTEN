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
    \brief Neighborhood Attention 3D - AV (attention * value) bindings
*/

#include <torch/extension.h>
#include <vector>

namespace natten {

// CPU forward declarations
torch::Tensor natten3dav_cpu_forward(
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation);

// CPU backward declarations
std::vector<torch::Tensor> natten3dav_cpu_backward(
    const torch::Tensor &d_out,
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation);

#if defined(WITH_CUDA)
// CUDA forward declarations
torch::Tensor natten3dav_cuda_forward(
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation);

torch::Tensor natten3dav_cuda_forward_fp16(
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation);

// CUDA backward declarations
std::vector<torch::Tensor> natten3dav_cuda_backward(
    const torch::Tensor &d_out,
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation);

std::vector<torch::Tensor> natten3dav_cuda_backward_fp16(
    const torch::Tensor &d_out,
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation);

#endif

// C++ interface
#define CHECK_CUDA(x) TORCH_CHECK(x.device().is_cuda(), #x " must be a CUDA tensor")
#define CHECK_CONTIGUOUS(x) TORCH_CHECK(x.is_contiguous(), #x " must be contiguous")

torch::Tensor natten3dav_forward(
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation) {
    CHECK_CONTIGUOUS(attn);
    CHECK_CONTIGUOUS(value);
    assert(int(sqrt(attn.size(5))) == kernel_size);
    assert(attn.device().is_cuda() == value.device().is_cuda());
    if (attn.device().is_cuda()) {
#if defined(WITH_CUDA)
        bool half = ::detail::scalar_type(value.scalar_type()) == at::ScalarType::Half;
        if (half)
            return natten3dav_cuda_forward_fp16(attn, value, kernel_size_d, kernel_size, dilation_d, dilation);
        return natten3dav_cuda_forward(attn, value, kernel_size_d, kernel_size, dilation_d, dilation);
#else
    AT_ERROR("NATTEN is not compiled with CUDA! Please make sure you installed correctly by referring to shi-labs.com/natten.");
#endif
    }
    return natten3dav_cpu_forward(attn, value, kernel_size_d, kernel_size, dilation_d, dilation);
}

std::vector<torch::Tensor> natten3dav_backward(
    const torch::Tensor &d_out,
    const torch::Tensor &attn,
    const torch::Tensor &value,
    const int kernel_size_d,
    const int kernel_size,
    const int dilation_d,
    const int dilation) {
    CHECK_CONTIGUOUS(d_out);
    CHECK_CONTIGUOUS(attn);
    CHECK_CONTIGUOUS(value);
    assert(int(sqrt(attn.size(5))) == kernel_size);
    assert(attn.device().is_cuda() == value.device().is_cuda() && d_out.device().is_cuda() == value.device().is_cuda());
    if (attn.device().is_cuda()) {
#if defined(WITH_CUDA)
        bool half = ::detail::scalar_type(value.scalar_type()) == at::ScalarType::Half;
        if (half)
            return natten3dav_cuda_backward_fp16(d_out, attn, value, kernel_size_d, kernel_size, dilation_d, dilation);
        return natten3dav_cuda_backward(d_out, attn, value, kernel_size_d, kernel_size, dilation_d, dilation);
#else
    AT_ERROR("NATTEN is not compiled with CUDA! Please make sure you installed correctly by referring to shi-labs.com/natten.");
#endif
    }
    return natten3dav_cpu_backward(d_out, attn, value, kernel_size_d, kernel_size, dilation_d, dilation);
}

} // namespace natten
