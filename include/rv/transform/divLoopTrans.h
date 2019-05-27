//===- rv/transform/divLoopTrans.h - make divergent loops uniform --*- C++ -*-===//
//
// Part of the RV Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//


#ifndef RV_TRANSFORM_DIVLOOPTRANS_H
#define RV_TRANSFORM_DIVLOOPTRANS_H


#include "rv/vectorizationInfo.h"
#include "llvm/ADT/DenseMap.h"
#include "llvm/IR/Instructions.h"
#include "llvm/ADT/SmallSet.h"



namespace llvm {
  class ConstantInt;
  class IntegerType;
  class LoopInfo;
  class DominatorTree;
  class PHINode;
}

namespace rv {

using PHIVec = llvm::SmallVector<llvm::PHINode*, 16>;
using PHISet = llvm::SmallPtrSet<llvm::PHINode*, 16>;
using BlockSet = llvm::SmallPtrSet<llvm::BasicBlock*, 4>;

class PlatformInfo;
class MaskExpander;
class LiveValueTracker;


struct TrackerDesc {
  llvm::PHINode * wrapPhi;    // kill exit live-out wrapper (@header)
  llvm::PHINode * trackerPhi; // divergent live out tracker (@header)
  llvm::PHINode * updatePhi;  // divergent live out updater (@pureLatch)

  TrackerDesc()
  : wrapPhi(nullptr)
  , trackerPhi(nullptr)
  , updatePhi(nullptr)
  {}
};

// Divergence tracker for a loop
struct TransformSession {
  llvm::Loop & loop;
  std::string loopName;
  llvm::LoopInfo & loopInfo;
  VectorizationInfo & vecInfo;
  PlatformInfo & platInfo;
  MaskExpander & maskEx;

  llvm::BasicBlock * testHead;
  llvm::BasicBlock * offsetHead;
  llvm::BasicBlock * pureLatch; // nullptr if the latch is not pure (yet)
  llvm::BasicBlock * oldLatch; // if pureLatch, then oldLatch is the unique predecessor to pureLatch

  // state tracking infrastructure
  TrackerDesc liveMaskDesc;
  // maps each exit block to the exit tracker in this loop
  llvm::DenseMap<const llvm::BasicBlock*, TrackerDesc> exitDescs;
  // maps each live out to a tracker
  llvm::DenseMap<const llvm::Value*, TrackerDesc> liveOutDescs;
  TrackerDesc & requestTrackerDesc(const llvm::Value& val); // creates an emty tracker if missing
  const TrackerDesc & getTrackerDesc(const llvm::Value& val) const; // asserting getter

  llvm::BasicBlock*
  remapExitingBlock(llvm::BasicBlock * exitingBlock) {
    if (exitingBlock == loop.getHeader()) return offsetHead;
    return exitingBlock;
  }

  size_t numKillExits;

  TransformSession(llvm::Loop & _loop, llvm::LoopInfo & _loopInfo, VectorizationInfo & _vecInfo, PlatformInfo & _platInfo, MaskExpander & _maskEx)
  : loop(_loop)
  , loopName(loop.getName().str())
  , loopInfo(_loopInfo)
  , vecInfo(_vecInfo)
  , platInfo(_platInfo)
  , maskEx(_maskEx)
  , testHead(nullptr)
  , offsetHead(nullptr)
  , pureLatch(nullptr)
  , oldLatch(nullptr)
  , liveMaskDesc()
  , numKillExits(0)
  {}

  // transform to a uniform loop
  // returns the canonical live mask phi
  void transformLoop();

  //
  void finalizeLiveOutTrackers();

  void finalizeLiveOutTracker(TrackerDesc & desc);

  llvm::BasicBlock & requestPureLatch();
};



// actual transformation
class DivLoopTrans {
  PlatformInfo & platInfo;
  VectorizationInfo & vecInfo;
  MaskExpander & maskEx;
  llvm::DominatorTree & domTree;
  llvm::LoopInfo & loopInfo;
  llvm::IntegerType * boolTy;
  // collect all divergent exits of this loop and send them through a dedicated latch exit

  llvm::DenseMap<const llvm::Loop*, TransformSession*> sessions;

// Control phase
  // return true, if any loops were transformed
  bool transformDivergentLoopControl(llvm::Loop & loop);

  // this finalizes the control conversion on @loop
  // void convertToLatchExitLoop(llvm::Loop & loop, LiveValueTracker & liveOutTracker);

// finalization phase
  // descend into all of @loop's loops and attach an input mask to the loop live mask phi
  void addLoopInitMasks(llvm::Loop & loop);

  // replace this value update phi with a proper blend cascade
public:
  DivLoopTrans(PlatformInfo & _platInfo, VectorizationInfo & _vecInfo, MaskExpander & _maskEx, llvm::DominatorTree & _domTree, llvm::LoopInfo & _loopInfo);
  ~DivLoopTrans();

  // makes all divergent loops in the region uniform
  void transformDivergentLoops();

  // stats
  size_t numDivergentLoops;
  size_t numKillExits;
};

}

#endif // RV_TRANSFORM_DIVLOOPTRANS_H
