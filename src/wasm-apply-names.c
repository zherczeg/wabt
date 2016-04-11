/*
 * Copyright 2016 WebAssembly Community Group participants
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "wasm-apply-names.h"

#include <assert.h>
#include <stdio.h>

#include "wasm-allocator.h"
#include "wasm-ast.h"

#define CHECK_ALLOC_(cond)                                             \
  do {                                                                 \
    if (!(cond)) {                                                     \
      fprintf(stderr, "%s:%d: allocation failed", __FILE__, __LINE__); \
      return WASM_ERROR;                                               \
    }                                                                  \
  } while (0)

#define CHECK_ALLOC(e) CHECK_ALLOC_(WASM_SUCCEEDED(e))
#define CHECK_ALLOC_NULL(v) CHECK_ALLOC_((v) != NULL)
#define CHECK_ALLOC_NULL_STR(v) CHECK_ALLOC_((v).start)

#define CHECK_RESULT(expr) \
  do {                     \
    if (WASM_FAILED(expr)) \
      return WASM_ERROR;   \
  } while (0)

typedef uint32_t WasmUint32;
WASM_DEFINE_VECTOR(uint32, WasmUint32);

typedef struct WasmContext {
  WasmAllocator* allocator;
  WasmModule* module;
  WasmFunc* current_func;
  WasmExprVisitor visitor;
  /* mapping from param index to its name, if any, for the current func */
  WasmStringSliceVector param_index_to_name;
  WasmStringSliceVector local_index_to_name;
} WasmContext;

static WasmResult use_name_for_var(WasmAllocator* allocator,
                                   WasmStringSlice* name,
                                   WasmVar* var) {
  if (var->type == WASM_VAR_TYPE_NAME) {
    assert(wasm_string_slices_are_equal(name, &var->name));
    return WASM_OK;
  }

  if (name->start) {
    var->type = WASM_VAR_TYPE_NAME;
    var->name = wasm_dup_string_slice(allocator, *name);
    return var->name.start != NULL ? WASM_OK : WASM_ERROR;
  }
  return WASM_OK;
}

static WasmResult use_name_for_func_type_var(WasmAllocator* allocator,
                                             WasmModule* module,
                                             WasmVar* var) {
  WasmFuncType* func_type = wasm_get_func_type_by_var(module, var);
  if (func_type == NULL)
    return WASM_ERROR;
  CHECK_ALLOC(use_name_for_var(allocator, &func_type->name, var));
  return WASM_OK;
}

static WasmResult use_name_for_func_var(WasmAllocator* allocator,
                                        WasmModule* module,
                                        WasmVar* var) {
  WasmFunc* func = wasm_get_func_by_var(module, var);
  if (func == NULL)
    return WASM_ERROR;
  CHECK_ALLOC(use_name_for_var(allocator, &func->name, var));
  return WASM_OK;
}

static WasmResult use_name_for_import_var(WasmAllocator* allocator,
                                          WasmModule* module,
                                          WasmVar* var) {
  WasmImport* import = wasm_get_import_by_var(module, var);
  if (import == NULL)
    return WASM_ERROR;
  CHECK_ALLOC(use_name_for_var(allocator, &import->name, var));
  return WASM_OK;
}

static WasmResult use_name_for_param_and_local_var(WasmContext* ctx,
                                                   WasmFunc* func,
                                                   WasmVar* var) {
  int local_index = wasm_get_local_index_by_var(func, var);
  assert(local_index >= 0 &&
         (size_t)local_index < wasm_get_num_params_and_locals(func));

  uint32_t num_params = wasm_get_num_params(func);
  WasmStringSlice* name;
  if ((uint32_t)local_index < num_params) {
    /* param */
    assert((size_t)local_index < ctx->param_index_to_name.size);
    name = &ctx->param_index_to_name.data[local_index];
  } else {
    /* local */
    local_index -= num_params;
    assert((size_t)local_index < ctx->local_index_to_name.size);
    name = &ctx->local_index_to_name.data[local_index];
  }

  if (var->type == WASM_VAR_TYPE_NAME) {
    assert(wasm_string_slices_are_equal(name, &var->name));
    return WASM_OK;
  }

  if (name->start) {
    var->type = WASM_VAR_TYPE_NAME;
    var->name = wasm_dup_string_slice(ctx->allocator, *name);
    return var->name.start != NULL ? WASM_OK : WASM_ERROR;
  }
  return WASM_OK;
}

static WasmResult begin_call_expr(WasmExpr* expr, void* user_data) {
  WasmContext* ctx = user_data;
  CHECK_RESULT(use_name_for_func_var(ctx->allocator, ctx->module,
                                     &expr->call.var));
  return WASM_OK;
}

static WasmResult begin_call_import_expr(WasmExpr* expr, void* user_data) {
  WasmContext* ctx = user_data;
  CHECK_RESULT(
      use_name_for_import_var(ctx->allocator, ctx->module, &expr->call.var));
  return WASM_OK;
}

static WasmResult begin_call_indirect_expr(WasmExpr* expr, void* user_data) {
  WasmContext* ctx = user_data;
  CHECK_RESULT(use_name_for_func_type_var(ctx->allocator, ctx->module,
                                          &expr->call_indirect.var));
  return WASM_OK;
}

static WasmResult on_get_local_expr(WasmExpr* expr, void* user_data) {
  WasmContext* ctx = user_data;
  CHECK_RESULT(use_name_for_param_and_local_var(ctx, ctx->current_func,
                                                &expr->get_local.var));
  return WASM_OK;
}

static WasmResult begin_set_local_expr(WasmExpr* expr, void* user_data) {
  WasmContext* ctx = user_data;
  CHECK_RESULT(use_name_for_param_and_local_var(ctx, ctx->current_func,
                                                &expr->set_local.var));
  return WASM_OK;
}

static WasmResult visit_func(WasmContext* ctx,
                             uint32_t func_index,
                             WasmFunc* func) {
  ctx->current_func = func;
  if (wasm_decl_has_func_type(&func->decl)) {
    CHECK_RESULT(use_name_for_func_type_var(ctx->allocator, ctx->module,
                                            &func->decl.type_var));
  }

  assert(wasm_decl_has_signature(&func->decl));

  CHECK_ALLOC(wasm_make_type_binding_reverse_mapping(
      ctx->allocator, &func->decl.sig.param_types, &func->param_bindings,
      &ctx->param_index_to_name));

  CHECK_ALLOC(wasm_make_type_binding_reverse_mapping(
      ctx->allocator, &func->local_types, &func->local_bindings,
      &ctx->local_index_to_name));

  CHECK_RESULT(wasm_visit_func(func, &ctx->visitor));
  ctx->current_func = NULL;
  return WASM_OK;
}

static WasmResult visit_import(WasmContext* ctx,
                               uint32_t import_index,
                               WasmImport* import) {
  if (wasm_decl_has_func_type(&import->decl)) {
    CHECK_RESULT(use_name_for_func_type_var(ctx->allocator, ctx->module,
                                            &import->decl.type_var));
  }
  return WASM_OK;
}

static WasmResult visit_export(WasmContext* ctx,
                               uint32_t export_index,
                               WasmExport* export) {
  CHECK_ALLOC(use_name_for_func_var(ctx->allocator, ctx->module, &export->var));
  return WASM_OK;
}

static WasmResult visit_module(WasmContext* ctx, WasmModule* module) {
  size_t i;
  for (i = 0; i < module->imports.size; ++i)
    CHECK_RESULT(visit_import(ctx, i, module->imports.data[i]));
  for (i = 0; i < module->funcs.size; ++i)
    CHECK_RESULT(visit_func(ctx, i, module->funcs.data[i]));
  for (i = 0; i < module->exports.size; ++i)
    CHECK_RESULT(visit_export(ctx, i, module->exports.data[i]));
  if (module->table) {
    for (i = 0; i < module->table->size; ++i) {
      CHECK_RESULT(use_name_for_func_var(ctx->allocator, ctx->module,
                                         &module->table->data[i]));
    }
  }
  return WASM_OK;
}


WasmResult wasm_apply_names(WasmAllocator* allocator, WasmModule* module) {
  WasmContext ctx;
  WASM_ZERO_MEMORY(ctx);
  ctx.allocator = allocator;
  ctx.module = module;
  ctx.visitor.user_data = &ctx;
  ctx.visitor.begin_call_expr = begin_call_expr;
  ctx.visitor.begin_call_import_expr = begin_call_import_expr;
  ctx.visitor.begin_call_indirect_expr = begin_call_indirect_expr;
  ctx.visitor.on_get_local_expr = on_get_local_expr;
  ctx.visitor.begin_set_local_expr = begin_set_local_expr;
  WasmResult result = visit_module(&ctx, module);
  wasm_destroy_string_slice_vector(allocator, &ctx.param_index_to_name);
  wasm_destroy_string_slice_vector(allocator, &ctx.local_index_to_name);
  return result;
}