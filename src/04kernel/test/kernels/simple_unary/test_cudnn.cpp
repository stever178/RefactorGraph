﻿#ifdef USE_CUDA

#include "../../../src/kernels/simple_unary/cpu_kernel.hh"
#include "../../../src/kernels/simple_unary/cudnn_activation_kernel.hh"
#include <gtest/gtest.h>

using namespace refactor;
using namespace kernel;

TEST(kernel, ActivationCudnn) {
    // build routine
    auto dataTensor = Tensor::share(DataType::F32, Shape{20, 30, 50});
    auto kCpu = SimpleUnaryCpu::build(SimpleUnaryType::Tanh, *dataTensor);
    ASSERT_TRUE(kCpu);
    auto kernel = ActivationCudnn::build(SimpleUnaryType::Tanh, *dataTensor);
    ASSERT_TRUE(kernel);
    auto rCpu = kCpu->lower();
    auto routine = kernel->lower();
    // malloc
    auto cpuMem = mem_manager::ForeignBlob::share(
        Target(Target::Cpu).memManager(),
        dataTensor->bytesSize());
    auto gpuMem = mem_manager::ForeignBlob::share(
        Target(Target::NvidiaGpu).memManager(),
        dataTensor->bytesSize());
    // put input data
    auto data = reinterpret_cast<float *>(cpuMem->operator void *());
    for (auto i : range0_(dataTensor->elementsSize())) { data[i] = i * 1e-4f; }
    gpuMem->copyIn(data, dataTensor->bytesSize());
    // inference
    auto res = runtime::Resources();
    {
        void const *inputs[]{*cpuMem};
        void *outputs[]{*cpuMem};
        rCpu(res, inputs, outputs);
    }
    {
        void const *inputs[]{*gpuMem};
        void *outputs[]{*gpuMem};
        routine(res, inputs, outputs);
    }
    // take output data
    std::vector<float> result(dataTensor->elementsSize());
    gpuMem->copyOut(result.data(), dataTensor->bytesSize());
    // check
    auto ans = reinterpret_cast<float const *>(cpuMem->operator void const *());
    for (auto i : range0_(result.size())) {
        EXPECT_FLOAT_EQ(ans[i], result[i]);
    }
}

#endif// USE_CUDA