﻿#include "infer.h"

namespace refactor::onnx {
    using namespace refactor::common;

    InferResult inferSoftmax(Operator const &op, Tensors inputs) {
        EXPECT_SIZE(1)
        if (!inputs[0]->dataType.isIeee754()) {
            return Err(InferError(ERROR_MSG("Input data type not support")));
        }
        return Ok(std::move(inputs));
    }
}// namespace refactor::onnx
