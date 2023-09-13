﻿#include "infer.h"

namespace refactor::onnx {
    using namespace refactor::common;

    InferResult inferArithmetic(Operator const &op, Tensors inputs) {
        EXPECT_SIZE(2) {
            auto const &a = inputs[0];
            auto const &b = inputs[1];
            auto dataType = a->dataType;
            if (!isNumbericDataType(dataType) || b->dataType != dataType) {
                return Err(InferError(ERROR_MSG("Data type not support")));
            }

            auto res = multidirBroadcast({a->shape, b->shape});
            if (res.isErr()) {
                return Err(InferError(ERROR_MSG(res.unwrapErr())));
            }
            auto output = std::move(res.unwrap());
            if (!shouldCalculate(inputs, output) || dataType != DataType::I64) {// Currently, only support int64_t
                return Ok(Tensors{Tensor::share(dataType, std::move(output))});
            }

            auto [shape, size] = shape_size(output);
            auto eleSize = dataTypeSize(dataType);
            auto blob = std::make_shared<Blob>(new uint8_t[size * eleSize]);
            auto dst = reinterpret_cast<uint64_t *>(blob->ptr);
            auto opType = op.opType.name();
            fmt::print("( {} dst<{}> = ", opType, size);
            for (size_t i = 0; i < size; ++i) {
                auto indices = buildIndices(shape, i);
                auto a_ = *reinterpret_cast<int64_t *>(locate(*a, indices)),
                     b_ = *reinterpret_cast<int64_t *>(locate(*b, indices));
                if (opType == "onnx::Add") {
                    dst[i] = a_ + b_;
                } else if (opType == "onnx::Sub") {
                    dst[i] = a_ - b_;
                } else if (opType == "onnx::Mul") {
                    dst[i] = a_ * b_;
                } else if (opType == "onnx::Div") {
                    dst[i] = a_ / b_;
                } else {
                    return Err(InferError(ERROR_MSG("OpType not support")));
                }
                fmt::print("{} ", dst[i]);
            }
            fmt::print(")");
            return Ok(Tensors{std::make_shared<Tensor>(dataType, std::move(output), std::move(blob))});
        }
    }
}// namespace refactor::onnx
