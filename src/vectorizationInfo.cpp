//===- vectorShape.h -----------------------------===//
//
//                     The Region Vectorizer
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// @author kloessner, simon
//

#include "rv/vectorizationInfo.h"

#include <llvm/IR/Instruction.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/Function.h>
#include <llvm/Analysis/LoopInfo.h>

#include "rv/region/Region.h"

using namespace llvm;

namespace rv
{

bool
VectorizationInfo::inRegion(const BasicBlock & block) const {
  return !region || region->contains(&block);
}

bool
VectorizationInfo::inRegion(const Instruction & inst) const {
  return !region || region->contains(inst.getParent());
}

void
VectorizationInfo::remapPredicate(Value& dest, Value& old)
{
    for (auto it : predicates)
    {
        if (it.second == &old)
        {
            predicates[it.first] = &dest;
        }
    }
}

void
VectorizationInfo::dump(const Value * val) const {
  llvm::raw_ostream & out = llvm::errs();
  if (!val) return;

  auto * block = dyn_cast<const BasicBlock>(val);
  if (block && inRegion(*block)) {
    dumpBlockInfo(*block);
  }

  if (hasKnownShape(*val)) {
  	out << *val << " : " << getVectorShape(*val).str() << "\n";
  } else {
  	out << *val << " : unknown shape\n";
  }
}

void
VectorizationInfo::dumpBlockInfo(const BasicBlock & block) const {
  llvm::raw_ostream & out = llvm::errs();
  const Value * predicate = getPredicate(block);

  out << "Block ";
  block.printAsOperand(out, false);
  out << ", predicate ";
  if (predicate) out << *predicate; else out << "null";
  out << "\n";

  for (const Instruction & inst : block) {
    dump(&inst);
  }
  out << "\n";
}

void VectorizationInfo::dumpArguments() const {
  llvm::raw_ostream& out = llvm::errs();
  const Function* F = mapping.scalarFn;

  out << "\nArguments:\n";

  for (auto& arg : F->args()) {
    out << arg << " : " << (hasKnownShape(arg) ? getVectorShape(arg).str() : "missing") << "\n";
  }

  out << "\n";
}

void
VectorizationInfo::dump() const
{
    llvm::raw_ostream& out = llvm::errs();
    out << "VectorizationInfo ";

    if (region) {
        out << "for Region " << region->str() << "\n";
    }
    else {
        out << "for function " << mapping.scalarFn->getName() << "\n";
    }

    dumpArguments();

    for (const BasicBlock & block : *mapping.scalarFn) {
      if (!inRegion(block)) continue;
      dumpBlockInfo(block);
    }

    out << "}\n";
}

VectorizationInfo::VectorizationInfo(llvm::Function& parentFn, uint vectorWidth, Region& _region)
: mapping(&parentFn, &parentFn, vectorWidth), region(&_region)
{
    mapping.resultShape = VectorShape::uni();
    for (auto& arg : parentFn.getArgumentList()) {
      mapping.argShapes.push_back(VectorShape::uni());
      setVectorShape(arg, VectorShape::uni());
    }
}

// VectorizationInfo
VectorizationInfo::VectorizationInfo(VectorMapping _mapping)
: mapping(_mapping), region(nullptr)
{
  assert(mapping.argShapes.size() == mapping.scalarFn->getArgumentList().size());
  auto& argList = mapping.scalarFn->getArgumentList();
  auto it = argList.begin();
  for (auto argShape : mapping.argShapes)
  {
    setVectorShape(*it, argShape);
    ++it;
  }
}

bool
VectorizationInfo::hasKnownShape(const llvm::Value& val) const {

  // explicit shape annotation take precedence
    if ((bool) shapes.count(&val)) return true;

  // in-region instruction must have an explicit shape
    auto * inst = dyn_cast<Instruction>(&val);
    if (inst && inRegion(*inst)) return false;

  // out-of-region values default to uniform
    return true;
}

VectorShape
VectorizationInfo::getVectorShape(const llvm::Value& val) const
{
 // return default shape for constants
    auto * constVal = dyn_cast<Constant>(&val);
    if (constVal) {
      return VectorShape::fromConstant(constVal);
    }

    auto it = shapes.find(&val);

  // for all other objects return a explicit shape first
    if (it != shapes.end()) {
      return it->second;
    }

  // out-of-region values default to uniform
    auto * inst = dyn_cast<Instruction>(&val);
    if (!inst || (inst && !inRegion(*inst))) {
      return VectorShape::uni(); // TODO getAlignment(*inst));
    }

    // return VectorShape::undef();
    assert (it != shapes.end());
    return it->second;
}

void
VectorizationInfo::dropVectorShape(const Value& val)
{
    auto it = shapes.find(&val);
    if (it == shapes.end()) return;
    shapes.erase(it);
}

void
VectorizationInfo::dropPredicate(const BasicBlock& block)
{
    auto it = predicates.find(&block);
    if (it == predicates.end()) return;
    predicates.erase(it);
}

void
VectorizationInfo::setVectorShape(const llvm::Value& val, VectorShape shape)
{
    shapes[&val] = shape;
}

llvm::Value*
VectorizationInfo::getPredicate(const llvm::BasicBlock& block) const
{
    auto it = predicates.find(&block);
    if (it == predicates.end())
    {
        return nullptr;
    }
    else
    {
        return it->second;
    }
}

void
VectorizationInfo::setPredicate(const llvm::BasicBlock& block, llvm::Value& predicate)
{
    predicates[&block] = &predicate;
}

void
VectorizationInfo::setLoopDivergence(const Loop & loop, bool toUniform) {
  mDivergentLoops.erase(&loop);
}

void
VectorizationInfo::setDivergentLoop(const Loop* loop)
{
    mDivergentLoops.insert(loop);
}

bool
VectorizationInfo::isDivergentLoop(const llvm::Loop* loop) const
{
    return static_cast<bool>(mDivergentLoops.count(loop));
}

bool
VectorizationInfo::isDivergentLoopTopLevel(const llvm::Loop* loop) const
{
    Loop* parent = loop->getParentLoop();

    return isDivergentLoop(loop) && (!parent || !isDivergentLoop(parent));
}


void
VectorizationInfo::markMandatory(const BasicBlock* BB)
{
    MandatoryBlocks.insert(BB);
}

bool
VectorizationInfo::isMandatory(const BasicBlock* BB) const
{
    return (bool) MandatoryBlocks.count(BB);
}

bool
VectorizationInfo::isKillExit(const BasicBlock& BB) const {
    return !isMandatory(&BB); // TODO figure this out directly in the VA and get rid of mandatory/optional
}

LLVMContext &
VectorizationInfo::getContext() const { return mapping.scalarFn->getContext(); }

BasicBlock&
VectorizationInfo::getEntry() const {
   if (region) return region->getRegionEntry();
   else return *mapping.scalarFn->begin();
 }


} /* namespace rv */


