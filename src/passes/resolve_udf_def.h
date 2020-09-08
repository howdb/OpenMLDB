/*-------------------------------------------------------------------------
 * Copyright (C) 2019, 4paradigm
 * resolve_udf_def.h
 *--------------------------------------------------------------------------
 **/
#ifndef SRC_PASSES_RESOLVE_UDF_DEF_H_
#define SRC_PASSES_RESOLVE_UDF_DEF_H_

#include <string>
#include <unordered_map>
#include <utility>
#include <vector>
#include "base/fe_status.h"
#include "node/sql_node.h"

namespace fesql {
namespace passes {

using base::Status;

class FnScopeInfo {
 public:
    Status AddVar(const std::string& var, int64_t expr_id) {
        auto iter = var_id_dict_.find(var);
        CHECK_TRUE(iter == var_id_dict_.end(), common::kCodegenError,
                   "Duplicate var def: ", var);
        var_id_dict_.insert(iter, std::make_pair(var, expr_id));
        return Status::OK();
    }

    int64_t GetVar(const std::string& var) {
        auto iter = var_id_dict_.find(var);
        if (iter == var_id_dict_.end()) {
            return -1;
        } else {
            return iter->second;
        }
    }

 private:
    std::unordered_map<std::string, int64_t> var_id_dict_;
};

/**
 * Check and resolve udf function def, that is,
 * all exprs in internal body are resolved and analyzed.
 */
class ResolveUdfDef {
 public:
    ResolveUdfDef() : scope_stack_(1) {}

    Status Visit(node::FnNodeFnDef* fn_def);

 private:
    Status Visit(node::FnNodeList* block);
    Status Visit(node::FnAssignNode* node);
    Status Visit(node::FnIfNode* node);
    Status Visit(node::FnElifNode* node);
    Status Visit(node::FnElseNode* node);
    Status Visit(node::FnIfBlock* node);
    Status Visit(node::FnElifBlock* node);
    Status Visit(node::FnElseBlock* node);
    Status Visit(node::FnIfElseBlock* node);
    Status Visit(node::FnForInBlock* node);
    Status Visit(node::FnReturnStmt* node);
    Status Visit(node::ExprNode* node);

    int64_t GetVar(const std::string& var);

    friend class FnScopeInfoGuard;
    FnScopeInfo* CurrentScope();
    std::vector<FnScopeInfo> scope_stack_;
};

class FnScopeInfoGuard {
 public:
    explicit FnScopeInfoGuard(ResolveUdfDef* parent) : parent_(parent) {
        parent_->scope_stack_.push_back(FnScopeInfo());
    }

    ~FnScopeInfoGuard() { parent_->scope_stack_.pop_back(); }

 private:
    ResolveUdfDef* parent_;
};

}  // namespace passes
}  // namespace fesql
#endif  // SRC_PASSES_RESOLVE_UDF_DEF_H_
