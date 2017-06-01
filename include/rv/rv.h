//===- rv.h ----------------*- C++ -*-===//
//
//                     The Region Vectorizer
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//

#ifndef RV_RV_H
#define RV_RV_H

#include "rv/PlatformInfo.h"
#include "rv/analysis/DFG.h"
#include "rv/transform/maskExpander.h"

#include "llvm/Transforms/Utils/ValueMapper.h"

namespace llvm {
  class LoopInfo;
  struct PostDominatorTree;
  class DominatorTree;
  class ScalarEvolution;
  class MemoryDependenceResults;
}

namespace rv {

class VectorizationInfo;

/*
 * The new vectorizer interface.
 *
 * The functionality of this interface relies heavily on prior loop simplification
 * (see LoopExitCanonicalizer), aswell as the elimination of critical edges beforehand.
 *
 * No guarantees are made in case the Function that is handled has critical edges,
 * or loop exits with with more than one predecessor are present.
 *
 * Vectorization of the Function in question may violate the original semantics, if
 * a wrong analysis is used as the VectorizationInfo object. The user is responsible for
 * running the analysis phase prior, or specifying a valid analysis himself.
 */
class VectorizerInterface {
public:
    VectorizerInterface(PlatformInfo & _platform);

    //
    // Analyze properties of the scalar function that are needed later in transformations
    // towards its SIMD equivalent.
    //
    // This expects initial information about arguments to be set in the VectorizationInfo object
    // (see VectorizationInfo).
    //
    void analyze(VectorizationInfo& vecInfo,
                 const llvm::CDG& cdg,
                 const llvm::DFG& dfg,
                 const llvm::LoopInfo& loopInfo,
                 const llvm::PostDominatorTree& postDomTree,
                 const llvm::DominatorTree& domTree);

    //
    // Linearize divergent regions of the scalar function to preserve semantics for the
    // vectorized function
    //
    bool
    linearize(VectorizationInfo& vecInfo,
                 CDG& cdg,
                 DFG& dfg,
                 LoopInfo& loopInfo,
                 PostDominatorTree& postDomTree,
                 DominatorTree& domTree);

    //
    // Produce vectorized instructions
    //
    bool
    vectorize(VectorizationInfo &vecInfo, const llvm::DominatorTree &domTree, const llvm::LoopInfo & loopInfo, llvm::ScalarEvolution & SE, llvm::MemoryDependenceResults & MDR, llvm::ValueToValueMapTy * vecInstMap);

    //
    // Ends the vectorization process on this function, removes metadata and
    // writes the function to a file
    //
    void finalize();

private:
    PlatformInfo & platInfo;

    void addIntrinsics();
};


   // implement all rv_* intrinsics
   // this is necessary to make scalar functions with predicate intrinsics executable
   // the SIMS semantics of the function will change if @scalar func used any mask intrinsics
  void lowerIntrinsics(llvm::Function & scalarFunc);
  void lowerIntrinsics(llvm::Module & mod);
}

#endif // RV_RV_H
