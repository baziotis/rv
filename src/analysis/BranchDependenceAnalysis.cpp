//===- BranchDependenceAnalysis.cpp ----------------*- C++ -*-===//
//
//                     The Region Vectorizer
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
// @authors simon
//
//

#include "rv/analysis/BranchDependenceAnalysis.h"

#include <llvm/IR/Dominators.h>
#include <llvm/IR/CFG.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/Analysis/LoopInfo.h>

#include "rvConfig.h"

using namespace llvm;

namespace rv {

ConstBlockSet BranchDependenceAnalysis::emptySet;

static void
IntersectInPlace(ConstBlockSet & x, const ConstBlockSet & y) {
  for (auto a : x) {
    if (!y.count(a)) x.erase(a);
  }
}

static ConstBlockSet
Intersect(const ConstBlockSet & x, const ConstBlockSet & y) {
  ConstBlockSet res;
  for (auto a : x) {
    if (y.count(a)) res.insert(a);
  }
  for (auto b : y) {
    if (x.count(b)) res.insert(b);
  }
  return res;
}

static void MergeIn(ConstBlockSet & m, ConstBlockSet & other) {
  for (auto b : other) m.insert(b);
}

static void
IntersectAndMerge(ConstBlockSet & accu, const ConstBlockSet & a, const ConstBlockSet & b) {
  for (auto x : a) {
    if (b.count(x)) {
      accu.insert(x);
    }
  }
}

static void
Subtract(ConstBlockSet & a, const ConstBlockSet & b) {
  for (auto y : b) {
    a.erase(y);
  }
}

static void DumpSet(const ConstBlockSet & blocks) {
  errs() << "{";
  for (const auto * bb : blocks) {
    errs() << ", " << bb->getName();
  }
  errs() << "}";
}

static void
GetDomRegion(DomTreeNodeBase<BasicBlock> & domNode, ConstBlockSet & domRegion) {
  domRegion.insert(domNode.getBlock());
  for (auto it = domNode.begin(); it != domNode.end(); ++it) {
    GetDomRegion(**it, domRegion);
  }
}

BranchDependenceAnalysis::BranchDependenceAnalysis(llvm::Function & F, const CDG & _cdg, const DFG & _dfg, const DominatorTree & _domTree, const PostDominatorTree & _postDomTree, const LoopInfo & _loopInfo)
: pdClosureMap()
, domClosureMap()
, effectedBlocks()
, cdg(_cdg)
, dfg(_dfg)
, domTree(_domTree)
, postDomTree(_postDomTree)
, loopInfo(_loopInfo)
{

  errs() << "-- frontiers --\n";
  for (auto & block : F) {

    ConstBlockSet pdClosure, domClosure;
    computePostDomClosure(block, pdClosure);
    computeDomClosure(block, domClosure);

    domClosure.insert(&block);
    pdClosure.insert(&block);

    errs() << block.getName() << " :\n\t DFG: ";
    DumpSet(domClosure);
    errs() << "\n\t PDF: ";
    DumpSet(pdClosure);
    errs() << "\n";
    pdClosureMap[&block] = pdClosure;
    domClosureMap[&block] = domClosure;
  }

#if 0 //  PDA divergence criterion
  for (auto & block : F) {
    const CDNode* cd_node = cdg[&block];
    if (!cd_node) continue;
    // Iterate over the predeccessors in the dfg, of the successors in the cdg
    for (const CDNode* cd_succ : cd_node->succs()) {
      const DFNode* df_node = dfg[cd_succ->getBB()];
      for (const DFNode* df_pred : df_node->preds()) {
        // Get the block BB that is affected by the varying branch
        const BasicBlock* const BB = df_pred->getBB();
        effectedBlocks[block.getTerminator()].insert(BB);
      }
    }
  }
#endif

  errs() << "-- branch dependence analysis --\n";
  F.dump();

  // maps phi blocks to divergence causing branches
  DenseMap<const BasicBlock*, ConstBlockSet> inverseMap;
// compute cd* for all blocks


  errs() << "-- branch dependence --\n";
  for (auto & z : F) {
    ConstBlockSet branchBlocks;

    for (auto itPred = pred_begin(&z), itEnd = pred_end(&z); itPred != itEnd; ++itPred) {
      auto * x = *itPred;
      auto xClosure = pdClosureMap[x];

      for (auto itOtherPred = pred_begin(&z); itOtherPred != itPred; ++itOtherPred) {
        auto * y = *itOtherPred;
        auto yClosure = pdClosureMap[y];

        errs() << "z = " << z.getName() << " with x = " << x->getName() << " , y = " << y->getName() << "\n";

        // early exit on: x reaches y or y reaches x
        if (yClosure.count(x)) {
          // x is in the PDF of y
          bool added = branchBlocks.insert(x).second;
          if (added) errs() << "\tx reaches y: add " << x->getName() << "\n";
        } else if (xClosure.count(y)) {
          // y is in the PDF of x
          bool added = branchBlocks.insert(y).second;
          if (added) errs() << "\ty reaches x: add " << y->getName() << "\n";
        }

        // intersection (set of binary branches that are reachable from both x and y)
        auto xyPostDomClosure = Intersect(yClosure, xClosure);

        // iterate over list of candidates
        for (auto * brBlock : xyPostDomClosure) {
          if (branchBlocks.count(brBlock)) continue; // already added by early exit

          errs() << "# disjoint paths from A = " << brBlock->getName() << " to z = " << z.getName() << "\n";

          // check if there can exist a disjoint path
          bool foundDisjointPath = false;

          // for all (disjoin) pairs of leaving edges
          for (auto itSucc = succ_begin(brBlock), itEndSucc = succ_end(brBlock); !foundDisjointPath && itSucc != itEndSucc; ++itSucc) {
            auto * b = *itSucc;
            auto bClosure = domClosureMap[b];

            for (auto itOtherSucc = succ_begin(brBlock); !foundDisjointPath && itOtherSucc != itSucc; ++itOtherSucc) {
              auto * c = *itOtherSucc;
              if (b == c) continue; // multi exits to the same target (switches)

              auto cClosure = domClosureMap[c];
              auto bcDomClosure = Intersect(bClosure, cClosure);

              if (bClosure.count(&z) && cClosure.count(&z)) {
                foundDisjointPath = true;
                break;
              }
            }
#if 0

            // early exit: check if a == x, b == y, ...
              if (a == y || b == y) {
                errs() << "\t a == y || b == y (OK)\n";
                foundDisjointPath = true;
                break;
              }

              errs() << "- b = " << b->getName() << "\n";

              auto * bPdNode = postDomTree.getNode(const_cast<BasicBlock*>(b));
              if (!bPdNode) continue;
              auto * bPd = bPdNode->getBlock();

              if (aPd == bPd) continue; // multi exits to the same post dom node (switches)

              // corner case: a == z
              if (aEqualsZ) {
                // b does not reach z
                if (!yClosure.count(bPd) && !xClosure.count(bPd)) continue;

                // if a reaches x, b must reach y (or vice versa)
              } else if (
                  !(aReachesX && yClosure.count(bPd)) ||
                  !(aReachesY && xClosure.count(bPd))) {
                continue;
              }

              errs() << "\t disjoint path\n";
              foundDisjointPath = true;
            }
#endif
          }

          // we found a disjoint path from brBlock to block
          if (foundDisjointPath) {
            errs() << "Adding " << brBlock->getName() << "\n";
            branchBlocks.insert(brBlock);
          }
        }
      }
    }

    inverseMap[&z] = branchBlocks;
  }
  errs () << "-- DONE --\n";


  IF_DEBUG {
    errs() << "-- Mapping of phi blocks to branches --\n";
    for (auto it : inverseMap) {
      errs() << it.first->getName() << " : ";
      DumpSet(it.second);
      errs() << "\n";
    }
  }

// taint LCSSA phis on loop exit divergence
  std::vector<Loop*> loopStack;
  for (auto * loop : loopInfo) loopStack.push_back(loop);

  while (!loopStack.empty()) {
    auto * loop = loopStack.back();
    loopStack.pop_back();

    for (auto * childLoop : *loop) {
      loopStack.push_back(childLoop);
    }

    // tain all exit blocks if any exit is divergent
    SmallVector<Edge, 4> exitEdges;
    loop->getExitEdges(exitEdges);

    for (auto exitEdge : exitEdges) {
      auto * lcPhiBlock = exitEdge.second;
      addDivergenceInducingExits(exitEdge, inverseMap[lcPhiBlock]);
    }
  }


// invert result for look up table
  for (auto it : inverseMap) {
    const auto * phiBlock = it.first;
    for (auto branchBlock : it.second) {
      auto * term = branchBlock->getTerminator();
      auto * branch = dyn_cast<BranchInst>(term);
      if (branch && !branch->isConditional()) continue; // non conditional branch
      if (!branch && !isa<SwitchInst>(term)) continue; // otw, must be a switch

      errs() << branchBlock->getName() << " inf -> " << phiBlock->getName() << "\n";
      effectedBlocks[term].insert(phiBlock);
    }
  }

// final result dump
  IF_DEBUG {
    errs() << "-- Mapping of br blocks to phi blocks --\n";
    for (auto it : effectedBlocks) {
      auto * brBlock = it.first->getParent();
      auto phiBlocks = it.second;
      errs() << brBlock->getName() << " : "; DumpSet(phiBlocks); errs() <<"\n";
    }
  }



  // dump PDA output for comparison
#if 1
  IF_DEBUG {
    errs() << "-- PDA output --\n";
    for (auto & block : F) {
      const CDNode* cd_node = cdg[&block];
      if (!cd_node) continue;

      bool printed = false;
      // Iterate over the predeccessors in the dfg, of the successors in the cdg
      ConstBlockSet seen;
      for (const CDNode* cd_succ : cd_node->succs()) {
        const DFNode* df_node = dfg[cd_succ->getBB()];
        for (const DFNode* df_pred : df_node->preds()) {
          // Get the block BB that is affected by the varying branch
          const BasicBlock* const BB = df_pred->getBB();
          if (!seen.insert(BB).second) continue;


          if (!printed) errs() << block.getName() << " : ";
          printed = true;

          errs() << ", " << BB->getName();
          // assert(effectedBlocks[block.getTerminator()].count(BB) && "missed a PDA block");
        }
      }
      if (printed) errs() << "\n";
    }
  }
#endif
}

/// \brief computes the iterated control dependence relation for @x
void
BranchDependenceAnalysis::computePostDomClosure(const BasicBlock & x, ConstBlockSet & closure) {
  auto * xcd = cdg[&x];
  if (!xcd) return;

  for (auto cd_succ : xcd->preds()) {
    auto * cdBlock = cd_succ->getBB();
    if (closure.insert(cdBlock).second) {
      computePostDomClosure(*cdBlock, closure);
    }
  }
}

void
BranchDependenceAnalysis::computeDomClosure(const BasicBlock & b, ConstBlockSet & closure) {
  auto * bdf = dfg[&b];
  if (!bdf) return;

  for (auto df_pred : bdf->preds()) {
    auto * dfBlock = df_pred->getBB();
    if (closure.insert(dfBlock).second) {
      computeDomClosure(*dfBlock, closure);
    }
  }
}


void
BranchDependenceAnalysis::addDivergenceInducingExits(Edge exitEdge, ConstBlockSet & branchBlocks) {
  assert(false && "not implemented!");
  abort();
}

} // namespace rv
