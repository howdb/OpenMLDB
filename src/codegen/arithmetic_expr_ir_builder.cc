/*-------------------------------------------------------------------------
 * Copyright (C) 2020, 4paradigm
 * arithmetic_expr_ir_builder.cc
 *
 * Author: chenjing
 * Date: 2020/1/8
 *--------------------------------------------------------------------------
 **/
#include "codegen/arithmetic_expr_ir_builder.h"
#include "codegen/cond_select_ir_builder.h"
#include "codegen/ir_base_builder.h"
#include "codegen/null_ir_builder.h"
#include "codegen/timestamp_ir_builder.h"
#include "codegen/type_ir_builder.h"

using fesql::common::kCodegenError;

namespace fesql {
namespace codegen {

ArithmeticIRBuilder::ArithmeticIRBuilder(::llvm::BasicBlock* block)
    : block_(block), cast_expr_ir_builder_(block) {}
ArithmeticIRBuilder::~ArithmeticIRBuilder() {}

Status ArithmeticIRBuilder::FDivTypeAccept(::llvm::Type* lhs,
                                           ::llvm::Type* rhs) {
    CHECK_TRUE(
        (TypeIRBuilder::IsNumber(lhs) || TypeIRBuilder::IsTimestampPtr(rhs)) &&
            TypeIRBuilder::IsNumber(rhs),
        kCodegenError, "Invalid FDiv type: lhs ", TypeIRBuilder::TypeName(lhs),
        " rhs ", TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
Status ArithmeticIRBuilder::SDivTypeAccept(::llvm::Type* lhs,
                                           ::llvm::Type* rhs) {
    CHECK_TRUE(TypeIRBuilder::IsNumber(lhs) && TypeIRBuilder::IsNumber(rhs),
               kCodegenError, "Invalid SDiv Op type: lhs ",
               TypeIRBuilder::TypeName(lhs), " rhs ",
               TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
Status ArithmeticIRBuilder::ModTypeAccept(::llvm::Type* lhs,
                                          ::llvm::Type* rhs) {
    CHECK_TRUE(TypeIRBuilder::IsNumber(lhs) && TypeIRBuilder::IsNumber(rhs),
               kCodegenError, "Invalid Mod Op type: lhs ",
               TypeIRBuilder::TypeName(lhs), " rhs ",
               TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
// Accept rules:
// 1. timestamp + timestamp
// 2. interger + timestamp
// 3. timestamp + integer
// 4. number + number
Status ArithmeticIRBuilder::AddTypeAccept(::llvm::Type* lhs,
                                          ::llvm::Type* rhs) {
    CHECK_TRUE(
        (TypeIRBuilder::IsTimestampPtr(lhs) &&
         TypeIRBuilder::IsTimestampPtr(rhs)) ||
            (TypeIRBuilder::IsTimestampPtr(lhs) &&
             TypeIRBuilder::IsInterger(rhs)) ||
            (TypeIRBuilder::IsInterger(lhs) &&
             TypeIRBuilder::IsTimestampPtr(rhs)) ||
            (TypeIRBuilder::IsNumber(lhs) && TypeIRBuilder::IsNumber(rhs)),
        kCodegenError, "Invalid Add Op type: lhs ",
        TypeIRBuilder::TypeName(lhs), " rhs ", TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
// Accept rules:
// 1. timestamp - interger
// 2. number - number
Status ArithmeticIRBuilder::SubTypeAccept(::llvm::Type* lhs,
                                          ::llvm::Type* rhs) {
    CHECK_TRUE(
        (TypeIRBuilder::IsTimestampPtr(lhs) &&
         TypeIRBuilder::IsInterger(rhs)) ||
            (TypeIRBuilder::IsNumber(lhs) && TypeIRBuilder::IsNumber(rhs)),
        kCodegenError, "Invalid Sub Op type: lhs ",
        TypeIRBuilder::TypeName(lhs), " rhs ", TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
Status ArithmeticIRBuilder::MultiTypeAccept(::llvm::Type* lhs,
                                            ::llvm::Type* rhs) {
    CHECK_TRUE(TypeIRBuilder::IsNumber(lhs) && TypeIRBuilder::IsNumber(rhs),
               kCodegenError, "Invalid Multi Op type: lhs ",
               TypeIRBuilder::TypeName(lhs), " rhs ",
               TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
Status AndTypeAccept(::llvm::Type* lhs, ::llvm::Type* rhs) {
    CHECK_TRUE(TypeIRBuilder::IsInterger(lhs) && TypeIRBuilder::IsInterger(rhs),
               kCodegenError, "Invalid And Op type: lhs ",
               TypeIRBuilder::TypeName(lhs), " rhs ",
               TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
Status LShiftTypeAccept(::llvm::Type* lhs, ::llvm::Type* rhs) {
    CHECK_TRUE(TypeIRBuilder::IsInterger(lhs) && TypeIRBuilder::IsInterger(rhs),
               kCodegenError, "Invalid Lshift Op type: lhs ",
               TypeIRBuilder::TypeName(lhs), " rhs ",
               TypeIRBuilder::TypeName(rhs))
    return Status::OK();
}
bool ArithmeticIRBuilder::InferAndCastedNumberTypes(
    ::llvm::BasicBlock* block, ::llvm::Value* left, ::llvm::Value* right,
    ::llvm::Value** casted_left, ::llvm::Value** casted_right,
    ::fesql::base::Status& status) {
    if (NULL == left || NULL == right) {
        status.msg = "left or right value is null";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }

    ::llvm::Type* left_type = left->getType();
    ::llvm::Type* right_type = right->getType();

    if (!TypeIRBuilder::IsNumber(left_type) ||
        !TypeIRBuilder::IsNumber(right_type)) {
        status.msg = "invalid type for arithmetic expression: " +
                     TypeIRBuilder::TypeName(left_type) + " and  " +
                     TypeIRBuilder::TypeName(right_type);
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }

    *casted_left = left;
    *casted_right = right;
    CastExprIRBuilder cast_expr_ir_builder(block);
    if (left_type != right_type) {
        if (cast_expr_ir_builder.IsSafeCast(left_type, right_type)) {
            if (!cast_expr_ir_builder.SafeCastNumber(left, right_type,
                                                     casted_left, status)) {
                status.msg = "fail to codegen add expr: " + status.msg;
                LOG(WARNING) << status.msg;
                return false;
            }
        } else if (cast_expr_ir_builder.IsSafeCast(right_type, left_type)) {
            if (!cast_expr_ir_builder.SafeCastNumber(right, left_type,
                                                     casted_right, status)) {
                status.msg = "fail to codegen add expr: " + status.msg;
                LOG(WARNING) << status.msg;
                return false;
            }
        } else if (cast_expr_ir_builder.IsIntFloat2PointerCast(left_type,
                                                               right_type)) {
            if (!cast_expr_ir_builder.UnSafeCastNumber(left, right_type,
                                                       casted_left, status)) {
                status.msg = "fail to codegen add expr: " + status.msg;
                LOG(WARNING) << status.msg;
                return false;
            }
        } else if (cast_expr_ir_builder.IsIntFloat2PointerCast(right_type,
                                                               left_type)) {
            if (!cast_expr_ir_builder.UnSafeCastNumber(right, left_type,
                                                       casted_right, status)) {
                status.msg = "fail to codegen add expr: " + status.msg;
                LOG(WARNING) << status.msg;
                return false;
            }
        } else {
            status.msg =
                "fail to codegen add expr: value type isn't compatible: " +
                TypeIRBuilder::TypeName(left_type) + " and  " +
                TypeIRBuilder::TypeName(right_type);
            status.code = common::kCodegenError;
            LOG(WARNING) << status;
            return false;
        }
    }
    return true;
}
bool ArithmeticIRBuilder::InferAndCastIntegerTypes(
    ::llvm::BasicBlock* block, ::llvm::Value* left, ::llvm::Value* right,
    ::llvm::Value** casted_left, ::llvm::Value** casted_right,
    ::fesql::base::Status& status) {
    if (NULL == left || NULL == right) {
        status.msg = "left or right value is null";
        status.code = common::kCodegenError;
        return false;
    }

    ::llvm::Type* left_type = left->getType();
    ::llvm::Type* right_type = right->getType();

    if (!left_type->isIntegerTy() || !right_type->isIntegerTy()) {
        status.msg = "invalid type for integer expression";
        status.code = common::kCodegenError;
        return false;
    }
    *casted_left = left;
    *casted_right = right;
    CastExprIRBuilder cast_expr_ir_builder(block);
    if (left_type != right_type) {
        if (cast_expr_ir_builder.IsSafeCast(left_type, right_type)) {
            if (!cast_expr_ir_builder.SafeCastNumber(left, right_type,
                                                     casted_left, status)) {
                status.msg = "fail to codegen add expr: " + status.msg;
                LOG(WARNING) << status.msg;
                return false;
            }
        } else if (cast_expr_ir_builder.IsSafeCast(right_type, left_type)) {
            if (!cast_expr_ir_builder.SafeCastNumber(right, left_type,
                                                     casted_right, status)) {
                status.msg = "fail to codegen add expr: " + status.msg;
                LOG(WARNING) << status.msg;
                return false;
            }
        } else {
            status.msg =
                "fail to codegen add expr: value type isn't compatible: " +
                TypeIRBuilder::TypeName(left_type) + " and  " +
                TypeIRBuilder::TypeName(right_type);
            status.code = common::kCodegenError;
            LOG(WARNING) << status;
            return false;
        }
    }
    return true;
}

bool ArithmeticIRBuilder::InferAndCastDoubleTypes(
    ::llvm::BasicBlock* block, ::llvm::Value* left, ::llvm::Value* right,
    ::llvm::Value** casted_left, ::llvm::Value** casted_right,
    ::fesql::base::Status& status) {
    if (NULL == left || NULL == right) {
        status.msg = "left or right value is null";
        status.code = common::kCodegenError;
        return false;
    }

    ::llvm::Type* left_type = left->getType();
    ::llvm::Type* right_type = right->getType();

    if (!TypeIRBuilder::IsNumber(left_type) ||
        !TypeIRBuilder::IsNumber(right_type)) {
        status.msg = "invalid type for arithmetic expression: " +
                     TypeIRBuilder::TypeName(left_type) + " " +
                     TypeIRBuilder::TypeName(right_type);
        status.code = common::kCodegenError;
        return false;
    }
    *casted_left = left;
    *casted_right = right;
    CastExprIRBuilder cast_expr_ir_builder(block);
    if (!left_type->isDoubleTy()) {
        if (!cast_expr_ir_builder.UnSafeCastNumber(
                left, ::llvm::Type::getDoubleTy(block->getContext()),
                casted_left, status)) {
            status.msg = "fail to codegen add expr: " + status.str();
            LOG(WARNING) << status;
            return false;
        }
    }

    if (!right_type->isDoubleTy()) {
        if (!cast_expr_ir_builder.UnSafeCastNumber(
                right, ::llvm::Type::getDoubleTy(block->getContext()),
                casted_right, status)) {
            status.msg = "fail to codegen add expr: " + status.str();
            LOG(WARNING) << status;
            return false;
        }
    }
    return true;
}

// Return left & right
bool ArithmeticIRBuilder::BuildAnd(::llvm::BasicBlock* block,
                                   ::llvm::Value* left, ::llvm::Value* right,
                                   ::llvm::Value** output,
                                   base::Status& status) {
    if (!left->getType()->isIntegerTy() || !right->getType()->isIntegerTy()) {
        status.msg =
            "fail to codegen arithmetic and expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    ::llvm::IRBuilder<> builder(block);
    *output = builder.CreateAnd(left, right);
    return true;
}
bool ArithmeticIRBuilder::BuildLShiftLeft(::llvm::BasicBlock* block,
                                          ::llvm::Value* left,
                                          ::llvm::Value* right,
                                          ::llvm::Value** output,
                                          base::Status& status) {
    if (!left->getType()->isIntegerTy() || !right->getType()->isIntegerTy()) {
        status.msg =
            "fail to codegen logical shift left expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    ::llvm::IRBuilder<> builder(block);
    *output = builder.CreateShl(left, right);
    return true;
}
bool ArithmeticIRBuilder::BuildLShiftRight(::llvm::BasicBlock* block,
                                           ::llvm::Value* left,
                                           ::llvm::Value* right,
                                           ::llvm::Value** output,
                                           base::Status& status) {
    if (!left->getType()->isIntegerTy() || !right->getType()->isIntegerTy()) {
        status.msg =
            "fail to codegen logical shift right expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    ::llvm::IRBuilder<> builder(block);
    *output = builder.CreateLShr(left, right);
    return true;
}
bool ArithmeticIRBuilder::BuildAddExpr(
    ::llvm::BasicBlock* block, ::llvm::Value* left, ::llvm::Value* right,
    ::llvm::Value** output,
    ::fesql::base::Status& status) {  // NOLINT

    status = AddTypeAccept(left->getType(), right->getType());
    if (!status.isOK()) {
        return false;
    }

    // Process timestamp add
    TimestampIRBuilder ts_builder(block->getModule());
    if (TypeIRBuilder::IsTimestampPtr(left->getType()) &&
        TypeIRBuilder::IsInterger(right->getType())) {
        status = ts_builder.TimestampAdd(block, left, right, output);
        return status.isOK();
    } else if (TypeIRBuilder::IsInterger(left->getType()) &&
               TypeIRBuilder::IsTimestampPtr(right->getType())) {
        status = ts_builder.TimestampAdd(block, right, left, output);
        return status.isOK();
    } else if (TypeIRBuilder::IsTimestampPtr(left->getType()) &&
               TypeIRBuilder::IsTimestampPtr(right->getType())) {
        ::llvm::Value* ts1;
        ::llvm::Value* ts2;
        ::llvm::Value* ts_add;
        if (!ts_builder.GetTs(block, left, &ts1)) {
            status.msg =
                "fail to codegen timestamp + timestamp expr: get lhs ts error";
            status.code = common::kCodegenError;
            return false;
        }
        if (!ts_builder.GetTs(block, right, &ts2)) {
            status.msg =
                "fail to codegen timestamp + timestamp expr: get rhs ts error";
            status.code = common::kCodegenError;
            return false;
        }
        if (!BuildAddExpr(block, ts1, ts2, &ts_add, status)) {
            return false;
        }
        if (!ts_builder.NewTimestamp(block, ts_add, output)) {
            status.msg =
                "fail to codegen timestamp + timestamp expr: new timestamp "
                "with ts error";
            status.code = common::kCodegenError;
            return false;
        }
        return true;
    }

    // Process number add
    ::llvm::Value* casted_left = NULL;
    ::llvm::Value* casted_right = NULL;
    if (!InferAndCastedNumberTypes(block, left, right, &casted_left,
                                   &casted_right, status)) {
        return false;
    }
    ::llvm::IRBuilder<> builder(block);
    if (casted_left->getType()->isIntegerTy()) {
        *output = builder.CreateAdd(casted_left, casted_right, "expr_add");
    } else if (casted_left->getType()->isFloatTy() ||
               casted_left->getType()->isDoubleTy()) {
        *output = builder.CreateFAdd(casted_left, casted_right, "expr_add");
    } else {
        status.msg = "fail to codegen add expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    return true;
}

Status ArithmeticIRBuilder::BuildAnd(const NativeValue& left,
                                     const NativeValue& right,
                                     NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildAnd(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildLShiftLeft(
    const NativeValue& left, const NativeValue& right,
    NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildLShiftLeft(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildLShiftRight(
    const NativeValue& left, const NativeValue& right,
    NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildLShiftRight(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildAddExpr(const NativeValue& left,
                                         const NativeValue& right,
                                         NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(AddTypeAccept(left.GetType(), right.GetType()))
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildAddExpr(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildSubExpr(const NativeValue& left,
                                         const NativeValue& right,
                                         NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(SubTypeAccept(left.GetType(), right.GetType()))
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildSubExpr(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildMultiExpr(
    const NativeValue& left, const NativeValue& right,
    NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(MultiTypeAccept(left.GetType(), right.GetType()))
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildMultiExpr(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildFDivExpr(
    const NativeValue& left, const NativeValue& right,
    NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(FDivTypeAccept(left.GetType(), right.GetType()))
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildFDivExpr(block, lhs, rhs, output, status);
        },
        value_output));
    if (value_output->IsConstNull()) {
        value_output->SetType(::llvm::Type::getDoubleTy(block_->getContext()));
    }
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildSDivExpr(
    const NativeValue& left, const NativeValue& right,
    NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(SDivTypeAccept(left.GetType(), right.GetType()))
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildSDivExpr(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
Status ArithmeticIRBuilder::BuildModExpr(const NativeValue& left,
                                         const NativeValue& right,
                                         NativeValue* value_output) {  // NOLINT
    CHECK_STATUS(ModTypeAccept(left.GetType(), right.GetType()))
    CHECK_STATUS(NullIRBuilder::SafeNullBinaryExpr(
        block_, left, right,
        [](::llvm::BasicBlock* block, ::llvm::Value* lhs, ::llvm::Value* rhs,
           ::llvm::Value** output, Status& status) {
            return BuildModExpr(block, lhs, rhs, output, status);
        },
        value_output));
    return Status::OK();
}
bool ArithmeticIRBuilder::BuildSubExpr(
    ::llvm::BasicBlock* block, ::llvm::Value* left, ::llvm::Value* right,
    ::llvm::Value** output,
    ::fesql::base::Status& status) {  // NOLINT

    ::llvm::IRBuilder<> builder(block);
    // Process timestamp add
    TimestampIRBuilder ts_builder(block->getModule());
    if (TypeIRBuilder::IsTimestampPtr(left->getType()) &&
        TypeIRBuilder::IsInterger(right->getType())) {
        ::llvm::Value* negative_value = nullptr;
        if (!BuildMultiExpr(block, builder.getInt16(-1), right, &negative_value,
                            status)) {
            return false;
        }
        status = ts_builder.TimestampAdd(block, left, negative_value, output);
        return status.isOK();
    }
    ::llvm::Value* casted_left = NULL;
    ::llvm::Value* casted_right = NULL;

    if (false == InferAndCastedNumberTypes(block, left, right, &casted_left,
                                           &casted_right, status)) {
        return false;
    }
    if (casted_left->getType()->isIntegerTy()) {
        *output = builder.CreateSub(casted_left, casted_right);
    } else if (casted_left->getType()->isFloatTy() ||
               casted_left->getType()->isDoubleTy()) {
        *output = builder.CreateFSub(casted_left, casted_right);
    } else {
        status.msg = "fail to codegen sub expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    return true;
}

bool ArithmeticIRBuilder::BuildMultiExpr(
    ::llvm::BasicBlock* block, ::llvm::Value* left, ::llvm::Value* right,
    ::llvm::Value** output,
    ::fesql::base::Status& status) {  // NOLINT

    ::llvm::Value* casted_left = NULL;
    ::llvm::Value* casted_right = NULL;

    if (false == InferAndCastedNumberTypes(block, left, right, &casted_left,
                                           &casted_right, status)) {
        return false;
    }
    ::llvm::IRBuilder<> builder(block);
    if (casted_left->getType()->isIntegerTy()) {
        *output = builder.CreateMul(casted_left, casted_right);
    } else if (casted_left->getType()->isFloatTy() ||
               casted_left->getType()->isDoubleTy()) {
        *output = builder.CreateFMul(casted_left, casted_right);
    } else {
        status.msg = "fail to codegen mul expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    return true;
}

bool ArithmeticIRBuilder::BuildFDivExpr(::llvm::BasicBlock* block,
                                        ::llvm::Value* left,
                                        ::llvm::Value* right,
                                        ::llvm::Value** output,
                                        base::Status& status) {
    ::llvm::Value* casted_left = NULL;
    ::llvm::Value* casted_right = NULL;

    if (false == InferAndCastDoubleTypes(block, left, right, &casted_left,
                                         &casted_right, status)) {
        if (TypeIRBuilder::IsTimestampPtr(left->getType())) {
            TimestampIRBuilder timestamp_ir_builder(block->getModule());
            timestamp_ir_builder.FDiv(block, left, right, output);
        }
        return false;
    }
    ::llvm::IRBuilder<> builder(block);
    if (casted_left->getType()->isFloatingPointTy()) {
        *output = builder.CreateFDiv(casted_left, casted_right);
    } else {
        status.msg = "fail to codegen fdiv expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    return true;
}
bool ArithmeticIRBuilder::BuildSDivExpr(::llvm::BasicBlock* block,
                                        ::llvm::Value* left,
                                        ::llvm::Value* right,
                                        ::llvm::Value** output,
                                        base::Status& status) {
    if (!left->getType()->isIntegerTy() || !right->getType()->isIntegerTy()) {
        status.msg =
            "fail to codegen integer sdiv expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    ::llvm::Value* casted_left = NULL;
    ::llvm::Value* casted_right = NULL;

    if (false == InferAndCastIntegerTypes(block, left, right, &casted_left,
                                          &casted_right, status)) {
        return false;
    }
    ::llvm::IRBuilder<> builder(block);

    // TODO(someone): fully and correctly handle arithmetic exception
    ::llvm::Type* llvm_ty = casted_right->getType();
    ::llvm::Value* zero = ::llvm::ConstantInt::get(llvm_ty, 0);
    ::llvm::Value* div_is_zero = builder.CreateICmpEQ(casted_right, zero);
    casted_right = builder.CreateSelect(
        div_is_zero, ::llvm::ConstantInt::get(llvm_ty, 1), casted_right);
    ::llvm::Value* div_result = builder.CreateSDiv(casted_left, casted_right);
    div_result = builder.CreateSelect(div_is_zero, zero, div_result);

    *output = div_result;
    return true;
}
bool ArithmeticIRBuilder::BuildModExpr(::llvm::BasicBlock* block,
                                       llvm::Value* left, llvm::Value* right,
                                       llvm::Value** output,
                                       base::Status status) {
    ::llvm::Value* casted_left = NULL;
    ::llvm::Value* casted_right = NULL;

    if (false == InferAndCastedNumberTypes(block, left, right, &casted_left,
                                           &casted_right, status)) {
        return false;
    }
    ::llvm::IRBuilder<> builder(block);
    if (casted_left->getType()->isIntegerTy()) {
        // TODO(someone): fully and correctly handle arithmetic exception
        ::llvm::Value* zero =
            ::llvm::ConstantInt::get(casted_right->getType(), 0);
        ::llvm::Value* rem_is_zero = builder.CreateICmpEQ(casted_right, zero);
        casted_right = builder.CreateSelect(
            rem_is_zero, ::llvm::ConstantInt::get(casted_right->getType(), 1),
            casted_right);
        ::llvm::Value* srem_result =
            builder.CreateSRem(casted_left, casted_right);
        srem_result = builder.CreateSelect(rem_is_zero, zero, srem_result);
        *output = srem_result;
    } else if (casted_left->getType()->isFloatingPointTy()) {
        *output = builder.CreateFRem(casted_left, casted_right);
    } else {
        status.msg = "fail to codegen mul expr: value types are invalid";
        status.code = common::kCodegenError;
        LOG(WARNING) << status;
        return false;
    }
    return true;
}

}  // namespace codegen
}  // namespace fesql
