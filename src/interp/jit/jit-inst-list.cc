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

#include "wabt/interp/jit/jit-inst-list.h"

namespace wabt {
namespace interp {

void ValueLocationAllocator::push(uint8_t reg, Type type) {
  uint16_t flags = 0;

  switch (type) {
  case Type::I32:
    break;
  case Type::I64:
    flags = ValueLocation::EightByteSize;
    break;
  case Type::F32:
    flags = ValueLocation::Float;
    break;
  case Type::F64:
    flags = ValueLocation::EightByteSize | ValueLocation::Float;
    break;
  case Type::V128:
    flags = ValueLocation::SixteenByteSize;
    break;
  case Type::FuncRef:
  case Type::ExternRef:
  case Type::Reference:
    flags = ValueLocation::Reference
      | (sizeof(void*) == 8 ? ValueLocation::EightByteSize : 0);
    break;
  default:
    WABT_UNREACHABLE;
  }

  size_t offset = 0;

  if (reg == 0) {
    // Allocate value at the end unless there is a free
    // space for the value. Allocating large values might
    // create free space for smaller values.

    if (flags & ValueLocation::SixteenByteSize) {
      if (size_ & (sizeof(v128) - 1)) {
        // Four byte alignment checked first
        // to ensure eight byte alignment.
        if (size_ & sizeof(uint32_t)) {
          assert(fourByteFreeSpaceEnd_ == 0);
          size_ += sizeof(uint32_t);
          fourByteFreeSpaceEnd_ = size_;
        }

        if (size_ & sizeof(uint64_t)) {
          assert(eightByteFreeSpaceEnd_ == 0);
          size_ += sizeof(uint64_t);
          eightByteFreeSpaceEnd_ = size_;
        }
      }

      offset = size_;
      size_ += sizeof(v128);
    } else if (flags & ValueLocation::EightByteSize) {
      if (eightByteFreeSpaceEnd_ != 0) {
        offset = eightByteFreeSpaceEnd_ - sizeof(uint64_t);
        eightByteFreeSpaceEnd_ = 0;
      } else {
        if (size_ & sizeof(uint32_t)) {
          assert(fourByteFreeSpaceEnd_ == 0);
          size_ += sizeof(uint32_t);
          fourByteFreeSpaceEnd_ = size_;
        }

        offset = size_;
        size_ += sizeof(uint64_t);
      }
    } else if (fourByteFreeSpaceEnd_ != 0) {
      offset = fourByteFreeSpaceEnd_ - sizeof(uint32_t);
      fourByteFreeSpaceEnd_ = 0;
    } else if (eightByteFreeSpaceEnd_ != 0) {
      offset = eightByteFreeSpaceEnd_ - sizeof(uint64_t);
      fourByteFreeSpaceEnd_ = eightByteFreeSpaceEnd_;
      eightByteFreeSpaceEnd_ = 0;
    } else {
      offset = size_;
      size_ += sizeof(uint32_t);
    }
  }

  values_.push_back(ValueLocation(offset, reg, type, flags));
}

void ValueLocationAllocator::pop() {
  assert(values_.size() > 0);

  if (values_.back().reg == 0) {
    size_t offset = values_.back().offset;
    uint16_t flags = values_.back().flags;

    if (flags & ValueLocation::SixteenByteSize) {
      assert(size_ == offset + sizeof(v128));
      size_ = offset;

      if (size_ > 0 && size_ == eightByteFreeSpaceEnd_) {
        size_ -= sizeof(uint64_t);
        eightByteFreeSpaceEnd_ = 0;
      }

      if (size_ > 0 && size_ == fourByteFreeSpaceEnd_) {
        size_ -= sizeof(uint32_t);
        fourByteFreeSpaceEnd_ = 0;
      }
    } else if (flags & ValueLocation::EightByteSize) {
      assert(eightByteFreeSpaceEnd_ == 0);

      if (size_ == offset + sizeof(uint64_t)) {
        size_ = offset;

        if (size_ > 0 && size_ == fourByteFreeSpaceEnd_) {
          size_ -= sizeof(uint32_t);
          fourByteFreeSpaceEnd_ = 0;
        }
      } else {
        eightByteFreeSpaceEnd_ = offset + sizeof(uint64_t);
      }
    } else {
      if (size_ == offset + sizeof(uint32_t)) {
        assert(fourByteFreeSpaceEnd_ == 0);
        size_ -= sizeof(uint32_t);
      } else if (offset + sizeof(uint64_t) == fourByteFreeSpaceEnd_) {
        assert(eightByteFreeSpaceEnd_ == 0);

        eightByteFreeSpaceEnd_ = fourByteFreeSpaceEnd_;
        fourByteFreeSpaceEnd_ = 0;
      } else {
        assert(fourByteFreeSpaceEnd_ == 0);
        fourByteFreeSpaceEnd_ = offset + sizeof(uint32_t);
      }
    }
  }

  values_.pop_back();
}

void InstructionList::clear() {
  for (auto it : labelStack_) {
    if (it->prev_ == nullptr) {
      delete it;
    }
  }

  labelStack_.clear();

  InstructionListItem* instr = first_;

  while (instr != nullptr) {
    InstructionListItem* next = instr->next();
    delete instr;
    instr = next;
  }

  first_ = nullptr;
  last_ = nullptr;
}

InstructionDesc* InstructionList::append(Opcode op) {
  InstructionDesc* instr = new InstructionDesc(op, last_);

  append(instr);
  return instr;
}

void InstructionList::appendBranch(Opcode opcode, Index depth) {
  assert(depth < labelStack_.size());

  LabelDesc* label = labelStack_[labelStack_.size() - (depth + 1)];
  InstructionDesc* branch = new InstructionDesc(opcode, last_);
  assert(branch->isBranch());
  branch->value().targetLabel = label;
  label->branches_.push_back(branch);
  append(branch);
}

void InstructionList::appendElseLabel() {
  LabelDesc* label = labelStack_[labelStack_.size() - 1];

  assert(label->branches().size() > 0);

  InstructionDesc* branch = label->branches()[0];
  label->branches_.erase(label->branches().begin());

  assert(branch->opcode() == Opcode::InterpBrUnless);

  label = new LabelDesc(last_);
  append(label);
  label->branches_.push_back(branch);
}

void InstructionList::pushLabel(bool isLoop) {
  LabelDesc* label = new LabelDesc(isLoop ? last_ : nullptr);
  labelStack_.push_back(label);

  if (isLoop) {
    append(label);
  }
}

void InstructionList::popLabel() {
  LabelDesc* label = labelStack_.back();
  labelStack_.pop_back();

  if (label->prev_ != nullptr) {
    // Loop instruction.
    InstructionDesc* branch = new InstructionDesc(Opcode::Br, last_);
    branch->value().targetLabel = label;
    label->branches_.push_back(branch);
    append(branch);
    return;
  }

  if (label->branches().size() == 0) {
    delete label;
  } else {
    append(label);
  }
}

void InstructionList::append(InstructionListItem* instr) {
  if (last_ != nullptr) {
    last_->next_ = instr;
  } else {
    first_ = instr;
  }

  last_ = instr;
}

}  // namespace interp
}  // namespace wabt
