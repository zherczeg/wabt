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

#ifndef WABT_JIT_INST_LIST_H_
#define WABT_JIT_INST_LIST_H_

#include "wabt/common.h"
#include "wabt/opcode.h"

namespace wabt {
namespace interp {

class InstructionDesc;
class LabelDesc;
class BranchDesc;

struct ValueLocation {
  static const uint16_t EightByteSize = 1 << 0;
  static const uint16_t SixteenByteSize = 1 << 1;
  static const uint16_t Float = 2 << 1;
  static const uint16_t Reference = 4 << 1;

  ValueLocation(size_t offset, uint8_t reg, int8_t type, uint16_t flags)
      : offset(offset)
      , reg(reg)
      , type(type)
      , flags(flags) {}

  size_t offset;
  uint8_t reg;
  int8_t type;
  uint16_t flags;
};

class ValueLocationAllocator {
 public:
  static const size_t alignment = sizeof(v128) - 1;

  void push(uint8_t reg, Type type);
  void pop();

  const std::vector<ValueLocation>& values() { return values_; }
  size_t alignedSize() {
    return (size_ + (alignment - 1)) & ~(alignment - 1);
  }

 private:
  std::vector<ValueLocation> values_;
  size_t size_ = 0;
  // Due to the allocation algorithm, at most one 4
  // and one 8 byte space can be free at any time.
  size_t fourByteFreeSpaceEnd_ = 0;
  size_t eightByteFreeSpaceEnd_ = 0;
};

class InstructionListItem {
  friend class InstructionList;

 public:
  enum class InstructionType {
    Instruction,
    Label,
  };

  virtual ~InstructionListItem() {}

  virtual InstructionType type() = 0;

  InstructionListItem* next() { return next_; }

  InstructionListItem* prev() { return prev_; }

  InstructionDesc& asInstruction() {
    assert(type() == InstructionType::Instruction);
    return *reinterpret_cast<InstructionDesc*>(this);
  }

  LabelDesc& asLabel() {
    assert(type() == InstructionType::Label);
    return *reinterpret_cast<LabelDesc*>(this);
  }

 protected:
  explicit InstructionListItem(InstructionListItem* prev)
      : next_(nullptr), prev_(prev) {}

 private:
  InstructionListItem* next_;
  InstructionListItem* prev_;
};

union InstructionValue {
  uint32_t value32;
  uint64_t value64;
  LabelDesc* targetLabel;
};

class InstructionDesc : public InstructionListItem {
  friend class InstructionList;

 public:
  InstructionType type() override { return InstructionType::Instruction; }

  Opcode opcode() { return opcode_; }

  bool isBranch() {
    return opcode_ == Opcode::Br || opcode_ == Opcode::BrIf ||
           opcode_ == Opcode::InterpBrUnless;
  }

  InstructionValue& value() { return value_; }

 private:
  explicit InstructionDesc(Opcode opcode, InstructionListItem* prev)
      : InstructionListItem(prev), opcode_(opcode) {}

  Opcode opcode_;
  InstructionValue value_;
};

class LabelDesc : public InstructionListItem {
  friend class InstructionList;

 public:
  InstructionType type() override { return InstructionType::Label; }
  const std::vector<InstructionDesc*>& branches() { return branches_; }

 private:
  explicit LabelDesc(InstructionListItem* prev) : InstructionListItem(prev) {}

  std::vector<InstructionDesc*> branches_;
};

class InstructionList {
 public:
  InstructionList() {}
  ~InstructionList() { clear(); }

  InstructionListItem* first() { return first_; }

  InstructionListItem* last() { return last_; }

  void clear();

  InstructionDesc* append(Opcode opcode);
  void appendBranch(Opcode opcode, Index depth);
  void appendElseLabel();

  void pushLabel(bool isLoop);
  void popLabel();

 private:
  void append(InstructionListItem* instr);

  InstructionListItem* first_ = nullptr;
  InstructionListItem* last_ = nullptr;

  std::vector<LabelDesc*> labelStack_;
};

}  // namespace interp
}  // namespace wabt

#endif  // WABT_JIT_INST_LIST_H_
