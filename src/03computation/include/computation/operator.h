﻿#ifndef COMPUTATION_OPERATOR_H
#define COMPUTATION_OPERATOR_H

#include "common/error_handler.h"
#include "layout.h"
#include <memory>

namespace refactor::computation {

    class Operator {
    public:
        virtual size_t opTypeId() const = 0;
        virtual std::string_view name() const = 0;
        virtual bool isLayoutDependent() const;
        virtual void transposeTo(LayoutType);

        template<class T> bool is() const {
            return opTypeId() == T::typeId();
        }
    };

    class AxisRankOperator : public Operator {
    public:
        uint32_t axis, rank;

        constexpr AxisRankOperator(uint32_t axis_, uint32_t rank_)
            : Operator(), axis(axis_), rank(rank_) {}
        bool isLayoutDependent() const final;
        void transposeTo(LayoutType target) final;
    };

    using SharedOp = std::shared_ptr<Operator>;

    struct Node {
        SharedOp op;
        std::string name;
    };

}// namespace refactor::computation

#endif// COMPUTATION_OPERATOR_H
