//===- MemoryAccessGrouper.cpp -----------------------------===//
//
//                     The Region Vectorizer
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// @author montada

#include "MemoryAccessGrouper.h"

#include <llvm/Analysis/ScalarEvolutionExpressions.h>

using namespace llvm;
using namespace native;

// MemoryGroup_BEGIN

typedef const SCEV *Element;

MemoryGroup::MemoryGroup(const SCEV *scev) :
  topIdx(1),
  elements(1, scev) {}

MemoryGroup::MemoryGroup() :
  topIdx(0),
  elements() {}

void MemoryGroup::insert(const SCEV *scev, int offset) {
  if (elements.empty()) {
    topIdx = 1;
    elements.push_back(scev);

  } else if (offset >= 0) {
    // resize if necessary
    if(offset > topIdx)
      elements.resize(std::max<unsigned>((unsigned) (elements.size() * 2), (unsigned) offset + 1), nullptr);

    elements[offset] = scev;
    topIdx = std::max<unsigned>(topIdx, (unsigned) offset + 1);

  } else if (offset < 0) {
    // shift everything to fit the element at the front
    unsigned shiftValue = (unsigned) -offset;
    unsigned oldSize = (unsigned) elements.size();
    elements.resize(elements.size() + shiftValue, nullptr);
    const SCEV **dataPtr = elements.data();
    memmove(dataPtr + shiftValue, dataPtr, oldSize * sizeof(Element));
    for (unsigned i = 1; i < shiftValue; ++i) {
      elements[i] = nullptr;
    }

    elements[0] = scev;
    topIdx = topIdx + shiftValue;
  }
}

// MemoryGroup_END

// MemoryAccessGrouper_BEGIN

MemoryAccessGrouper::MemoryAccessGrouper(ScalarEvolution &SE, unsigned laneByteSize) :
  SE(SE),
  laneByteSize(laneByteSize),
  laneFloatTy(laneByteSize == 4 ? Type::getFloatTy(SE.getContext())
                                : Type::getDoubleTy(SE.getContext())),
  laneIntTy(Type::getIntNTy(SE.getContext(), laneByteSize * 8)) {
  assert(laneByteSize == 4 || laneByteSize == 8 && "unsupported lane size");
}

const SCEV *MemoryAccessGrouper::add(Value *addrVal) {
  const SCEV *addrSCEV = SE.getSCEV(addrVal);
  assert(addrSCEV && "can't compute SCEV");

  // try to find existing group with constant offset to addrSCEV
  for (MemoryGroup &group : memoryGroups) {
    int offset = 0;
    if (!getConstantOffset(addrSCEV, group[0], offset))
      continue;

    group.insert(addrSCEV, offset);
    return addrSCEV;
  }

  // new group
  MemoryGroup freshGroup(addrSCEV);
  memoryGroups.push_back(freshGroup);
  return addrSCEV;
}

bool MemoryAccessGrouper::getConstantOffset(const SCEV *a, const SCEV *b, int &offset) {
  Type *floatTy = Type::getFloatTy(SE.getContext());
  Type *i32Ty = Type::getInt32Ty(SE.getContext());

  const SCEV *diffSCEV = SE.getMinusSCEV(a, b);

  const SCEVUnknown *unknownSCEV = dyn_cast<SCEVUnknown>(diffSCEV);
  if (unknownSCEV && (unknownSCEV->isSizeOf(floatTy) || unknownSCEV->isSizeOf(i32Ty))) {
    offset = 1;
    return true;
  }

  auto *constantSCEV = dyn_cast<SCEVConstant>(diffSCEV);
  if (constantSCEV) {
    int byteOffset = (int) constantSCEV->getValue()->getLimitedValue();
    if (byteOffset % laneByteSize != 0)
      return false;
    offset = byteOffset / laneByteSize;

    return true;
  }

  const SCEVMulExpr *mulSCEV = dyn_cast<SCEVMulExpr>(diffSCEV);
  if (!mulSCEV)
    return false;

  const SCEV *leftOp = mulSCEV->getOperand(0);
  const SCEV *rightOp = mulSCEV->getOperand(1);
  const auto *leftConst = dyn_cast<SCEVConstant>(leftOp);
  if (!leftConst)
    return false;

  const SCEVUnknown *rightUnknown = dyn_cast<SCEVUnknown>(rightOp);
  if (!rightUnknown)
    return false;

  bool hasComponentStride = rightUnknown->isSizeOf(laneFloatTy) || rightUnknown->isSizeOf(laneIntTy);
  if (!hasComponentStride)
    return false;

  offset = (int) leftConst->getValue()->getLimitedValue();
  return true;
}

// MemoryAccessGrouper_END