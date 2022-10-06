/*
 * Copyright 2020 WebAssembly Community Group participants
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

#include "wabt/interp/jit/binary-reader-jit.h"
#include "wabt/binary-reader-nop.h"
#include "wabt/interp/jit/jit-inst-list.h"
#include "wabt/interp/jit/jit.h"
#include "wabt/shared-validator.h"

namespace wabt {
namespace interp {

class BinaryReaderJIT : public BinaryReaderNop {
 public:
  BinaryReaderJIT(ModuleDesc* module,
                  std::string_view filename,
                  Errors* errors,
                  const Features& features);

  // Implement BinaryReader.
  bool OnError(const Error&) override;

  Result EndModule() override;

  Result OnTypeCount(Index count) override;
  Result OnFuncType(Index index,
                    Index param_count,
                    Type* param_types,
                    Index result_count,
                    Type* result_types) override;

#if 0
  Result OnImportFunc(Index import_index,
                      std::string_view module_name,
                      std::string_view field_name,
                      Index func_index,
                      Index sig_index) override;
  Result OnImportTable(Index import_index,
                       std::string_view module_name,
                       std::string_view field_name,
                       Index table_index,
                       Type elem_type,
                       const Limits* elem_limits) override;
  Result OnImportMemory(Index import_index,
                        std::string_view module_name,
                        std::string_view field_name,
                        Index memory_index,
                        const Limits* page_limits) override;
  Result OnImportGlobal(Index import_index,
                        std::string_view module_name,
                        std::string_view field_name,
                        Index global_index,
                        Type type,
                        bool mutable_) override;
  Result OnImportTag(Index import_index,
                     std::string_view module_name,
                     std::string_view field_name,
                     Index tag_index,
                     Index sig_index) override;

  Result OnFunctionCount(Index count) override;
  Result OnFunction(Index index, Index sig_index) override;

  Result OnTableCount(Index count) override;
  Result OnTable(Index index,
                 Type elem_type,
                 const Limits* elem_limits) override;

  Result OnMemoryCount(Index count) override;
  Result OnMemory(Index index, const Limits* limits) override;

  Result OnGlobalCount(Index count) override;
  Result BeginGlobal(Index index, Type type, bool mutable_) override;
  Result BeginGlobalInitExpr(Index index) override;
  Result EndGlobalInitExpr(Index index) override;

  Result OnTagCount(Index count) override;
  Result OnTagType(Index index, Index sig_index) override;

  Result OnExport(Index index,
                  ExternalKind kind,
                  Index item_index,
                  std::string_view name) override;

  Result OnStartFunction(Index func_index) override;

#endif
  Result BeginFunctionBody(Index index, Offset size) override;
#if 0
  Result OnLocalDeclCount(Index count) override;
  Result OnLocalDecl(Index decl_index, Index count, Type type) override;

  Result OnOpcode(Opcode Opcode) override;
  Result OnAtomicLoadExpr(Opcode opcode,
                          Index memidx,
                          Address alignment_log2,
                          Address offset) override;
  Result OnAtomicStoreExpr(Opcode opcode,
                           Index memidx,
                           Address alignment_log2,
                           Address offset) override;
  Result OnAtomicRmwExpr(Opcode opcode,
                         Index memidx,
                         Address alignment_log2,
                         Address offset) override;
  Result OnAtomicRmwCmpxchgExpr(Opcode opcode,
                                Index memidx,
                                Address alignment_log2,
                                Address offset) override;
  Result OnAtomicWaitExpr(Opcode opcode,
                          Index memidx,
                          Address alignment_log2,
                          Address offset) override;
  Result OnAtomicFenceExpr(uint32_t consistency_model) override;
  Result OnAtomicNotifyExpr(Opcode opcode,
                            Index memidx,
                            Address alignment_log2,
                            Address offset) override;
  Result OnBinaryExpr(Opcode opcode) override;
#endif
  Result OnBlockExpr(Type sig_type) override;
  Result OnBrExpr(Index depth) override;
  Result OnBrIfExpr(Index depth) override;
#if 0
  Result OnBrTableExpr(Index num_targets,
                       Index* target_depths,
                       Index default_target_depth) override;
  Result OnCallExpr(Index func_index) override;
  Result OnCallIndirectExpr(Index sig_index, Index table_index) override;
  Result OnCatchExpr(Index tag_index) override;
  Result OnCatchAllExpr() override;
  Result OnDelegateExpr(Index depth) override;
  Result OnReturnCallExpr(Index func_index) override;
  Result OnReturnCallIndirectExpr(Index sig_index, Index table_index) override;
  Result OnCompareExpr(Opcode opcode) override;
  Result OnConvertExpr(Opcode opcode) override;
  Result OnDropExpr() override;
#endif
  Result OnElseExpr() override;
  Result OnEndExpr() override;
#if 0
  Result OnF32ConstExpr(uint32_t value_bits) override;
  Result OnF64ConstExpr(uint64_t value_bits) override;
  Result OnV128ConstExpr(v128 value_bits) override;
  Result OnGlobalGetExpr(Index global_index) override;
  Result OnGlobalSetExpr(Index global_index) override;
#endif
  Result OnI32ConstExpr(uint32_t value) override;
  Result OnI64ConstExpr(uint64_t value) override;
  Result OnIfExpr(Type sig_type) override;
#if 0
  Result OnLoadExpr(Opcode opcode,
                    Index memidx,
                    Address alignment_log2,
                    Address offset) override;
  Result OnLocalGetExpr(Index local_index) override;
  Result OnLocalSetExpr(Index local_index) override;
  Result OnLocalTeeExpr(Index local_index) override;
#endif
  Result OnLoopExpr(Type sig_type) override;
#if 0
  Result OnMemoryCopyExpr(Index srcmemidx, Index destmemidx) override;
  Result OnDataDropExpr(Index segment_index) override;
  Result OnMemoryGrowExpr(Index memidx) override;
  Result OnMemoryFillExpr(Index memidx) override;
  Result OnMemoryInitExpr(Index segment_index, Index memidx) override;
  Result OnMemorySizeExpr(Index memidx) override;
  Result OnRefFuncExpr(Index func_index) override;
  Result OnRefNullExpr(Type type) override;
  Result OnRefIsNullExpr() override;
  Result OnNopExpr() override;
  Result OnRethrowExpr(Index depth) override;
  Result OnReturnExpr() override;
  Result OnSelectExpr(Index result_count, Type* result_types) override;
  Result OnStoreExpr(Opcode opcode,
                     Index memidx,
                     Address alignment_log2,
                     Address offset) override;
  Result OnUnaryExpr(Opcode opcode) override;
  Result OnTableCopyExpr(Index dst_index, Index src_index) override;
  Result OnTableGetExpr(Index table_index) override;
  Result OnTableSetExpr(Index table_index) override;
  Result OnTableGrowExpr(Index table_index) override;
  Result OnTableSizeExpr(Index table_index) override;
  Result OnTableFillExpr(Index table_index) override;
  Result OnElemDropExpr(Index segment_index) override;
  Result OnTableInitExpr(Index segment_index, Index table_index) override;
  Result OnTernaryExpr(Opcode opcode) override;
  Result OnThrowExpr(Index tag_index) override;
  Result OnTryExpr(Type sig_type) override;
  Result OnUnreachableExpr() override;
#endif
  Result EndFunctionBody(Index index) override;
#if 0
  Result OnSimdLaneOpExpr(Opcode opcode, uint64_t value) override;
  Result OnSimdLoadLaneExpr(Opcode opcode,
                            Index memidx,
                            Address alignment_log2,
                            Address offset,
                            uint64_t value) override;
  Result OnSimdStoreLaneExpr(Opcode opcode,
                             Index memidx,
                             Address alignment_log2,
                             Address offset,
                             uint64_t value) override;
  Result OnSimdShuffleOpExpr(Opcode opcode, v128 value) override;
  Result OnLoadSplatExpr(Opcode opcode,
                         Index memidx,
                         Address alignment_log2,
                         Address offset) override;
  Result OnLoadZeroExpr(Opcode opcode,
                        Index memidx,
                        Address alignment_log2,
                        Address offset) override;

  Result OnElemSegmentCount(Index count) override;
  Result BeginElemSegment(Index index,
                          Index table_index,
                          uint8_t flags) override;
  Result BeginElemSegmentInitExpr(Index index) override;
  Result EndElemSegmentInitExpr(Index index) override;
  Result OnElemSegmentElemType(Index index, Type elem_type) override;
  Result OnElemSegmentElemExprCount(Index index, Index count) override;
  Result OnElemSegmentElemExpr_RefNull(Index segment_index, Type type) override;
  Result OnElemSegmentElemExpr_RefFunc(Index segment_index,
                                       Index func_index) override;

  Result OnDataCount(Index count) override;
  Result BeginDataSegmentInitExpr(Index index) override;
  Result EndDataSegmentInitExpr(Index index) override;
  Result BeginDataSegment(Index index,
                          Index memory_index,
                          uint8_t flags) override;
  Result OnDataSegmentData(Index index,
                           const void* data,
                           Address size) override;
#endif
 private:
  Location GetLocation() const;

  Errors* errors_ = nullptr;
  ModuleDesc& module_;

  SharedValidator validator_;
  std::string_view filename_;

  InstructionList instrList;
};

Location BinaryReaderJIT::GetLocation() const {
  Location loc;
  loc.filename = filename_;
  loc.offset = state->offset;
  return loc;
}

BinaryReaderJIT::BinaryReaderJIT(ModuleDesc* module,
                                 std::string_view filename,
                                 Errors* errors,
                                 const Features& features)
    : errors_(errors),
      module_(*module),
      validator_(errors, ValidateOptions(features)),
      filename_(filename) {}

bool BinaryReaderJIT::OnError(const Error& error) {
  errors_->push_back(error);
  return true;
}

Result BinaryReaderJIT::EndModule() {
  CHECK_RESULT(validator_.EndModule());
  return Result::Ok;
}

Result BinaryReaderJIT::OnTypeCount(Index count) {
  return Result::Ok;
}

Result BinaryReaderJIT::OnFuncType(Index index,
                                   Index param_count,
                                   Type* param_types,
                                   Index result_count,
                                   Type* result_types) {
  CHECK_RESULT(validator_.OnFuncType(GetLocation(), param_count, param_types,
                                     result_count, result_types, index));
  return Result::Ok;
}

Result BinaryReaderJIT::BeginFunctionBody(Index index, Offset size) {
  printf("Start compiling jit\n");
  instrList.pushLabel(false);
  return Result::Ok;
}

Result BinaryReaderJIT::OnBlockExpr(Type sig_type) {
  instrList.pushLabel(false);
  return Result::Ok;
}

Result BinaryReaderJIT::OnBrExpr(Index depth) {
  instrList.appendBranch(Opcode::Br, depth);
  return Result::Ok;
}

Result BinaryReaderJIT::OnBrIfExpr(Index depth) {
  instrList.appendBranch(Opcode::BrIf, depth);
  return Result::Ok;
}

Result BinaryReaderJIT::OnElseExpr() {
  instrList.appendElseLabel();
  return Result::Ok;
}

Result BinaryReaderJIT::OnEndExpr() {
  instrList.popLabel();
  return Result::Ok;
}

Result BinaryReaderJIT::OnI32ConstExpr(uint32_t value) {
  InstructionDesc* instr = instrList.append(Opcode::I32Const);
  instr->value().value32 = value;
  return Result::Ok;
}

Result BinaryReaderJIT::OnI64ConstExpr(uint64_t value) {
  InstructionDesc* instr = instrList.append(Opcode::I64Const);
  instr->value().value64 = value;
  return Result::Ok;
}

Result BinaryReaderJIT::OnIfExpr(Type sig_type) {
  instrList.pushLabel(false);
  instrList.appendBranch(Opcode::InterpBrUnless, 0);
  return Result::Ok;
}

Result BinaryReaderJIT::OnLoopExpr(Type sig_type) {
  instrList.pushLabel(true);
  return Result::Ok;
}

Result BinaryReaderJIT::EndFunctionBody(Index index) {
  printf("End compiling jit\n");

  for (InstructionListItem* item = instrList.first(); item != nullptr;
       item = item->next()) {
    if (item->type() == InstructionListItem::InstructionType::Instruction) {
      InstructionDesc& instr = item->asInstruction();
      printf("%p instruction: Opcode: %s\n", item, instr.opcode().GetName());
      if (instr.isBranch()) {
        printf("  Jump to: %p\n", instr.value().targetLabel);
      }
    } else {
      printf("%p label:\n", item);
      for (auto it : item->asLabel().branches()) {
        printf("  Jump from: %p\n", it);
      }
    }
  }

  instrList.clear();
  return Result::Ok;
}

Result ReadBinaryJIT(std::string_view filename,
                     const void* data,
                     size_t size,
                     const ReadBinaryOptions& options,
                     Errors* errors,
                     ModuleDesc* out_module) {
  BinaryReaderJIT reader(out_module, filename, errors, options.features);
  return ReadBinary(data, size, &reader, options);
}

}  // namespace interp
}  // namespace wabt
