#include "llvm/ADT/DenseMap.h"
#include "llvm/Analysis/IteratedDominanceFrontier.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/Dominators.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Pass.h"
#include "llvm/Passes/PassBuilder.h"
#include "llvm/Passes/PassPlugin.h"

using namespace llvm;
namespace {
struct OurMemToReg : public PassInfoMixin<OurMemToReg> {
  struct VariableInfo {
    VariableInfo(AllocaInst *AI) : Alloca(AI) {}
    AllocaInst *Alloca;
    SmallPtrSet<BasicBlock *, 32> DefBlocks;
    std::vector<Value *> DefStack;
  };
  std::vector<Instruction *> TrashList;
  std::vector<std::unique_ptr<VariableInfo>> VariableInfos;
  DenseMap<Instruction *, VariableInfo *> InstToVariableInfo;
  static char ID; // Pass identification, replacement for typeid

  OurMemToReg() {}
  void linkDefsAndUsesToVar(VariableInfo *VarInfo) {
    for (auto *Use : VarInfo->Alloca->users()) {
      if (auto *Load = dyn_cast<LoadInst>(Use)) {
        InstToVariableInfo[Load] = VarInfo;
      } else if (auto *Store = dyn_cast<StoreInst>(Use)) {
        InstToVariableInfo[Store] = VarInfo;
        VarInfo->DefBlocks.insert(Store->getParent());
      }
    }
  }

  static bool escapes(AllocaInst *Alloc) {
    for (auto *Use : Alloc->users()) {
      if (isa<LoadInst>(Use))
        continue;
      if (auto *Store = dyn_cast<StoreInst>(Use)) {
        if (Store->getValueOperand() == Alloc) {
          return true;
        }
        continue;
      }
      return true;
    }
    return false;
  }

  void renameRecursive(DomTreeNode *DN) {
    BasicBlock &BB = *DN->getBlock();

    for (auto &Inst : BB) {
      if (auto *VarInfo = InstToVariableInfo[&Inst]) {
        if (isa<StoreInst>(&Inst)) {
          VarInfo->DefStack.push_back(Inst.getOperand(0));
        } else if (isa<LoadInst>(&Inst)) {
          if (!VarInfo->DefStack.empty()) {
            Inst.replaceAllUsesWith(VarInfo->DefStack.back());
          }
        } else if (isa<PHINode>(&Inst)) {
          VarInfo->DefStack.push_back(&Inst);
        }
      }
    }

    for (auto *Succ : successors(&BB)) {
      for (Instruction &InstRef : *Succ) {
        if (auto *Phi = dyn_cast<PHINode>(&InstRef)) {
          if (auto *VarInfo = InstToVariableInfo[&InstRef]) {
            if (!VarInfo->DefStack.empty()) {
              Phi->addIncoming(VarInfo->DefStack.back(), &BB);
            }
          }
        }
      }
    }

    for (auto DNChild : DN->children()) {
      renameRecursive(DNChild);
    }

    for (Instruction &InstRef : BB) {
      Instruction *Inst = &InstRef;
      if (auto *VarInfo = InstToVariableInfo[Inst]) {
        if (isa<StoreInst>(Inst)) {
          VarInfo->DefStack.pop_back();
          TrashList.push_back(Inst);
        } else if (isa<PHINode>(Inst)) {
          VarInfo->DefStack.pop_back();
        } else if (isa<LoadInst>(Inst)) {
          TrashList.push_back(Inst);
        }
      }
    }

  }

  PreservedAnalyses run(Module &M, ModuleAnalysisManager &AM) {
    FunctionAnalysisManager &FAM =
        AM.getResult<FunctionAnalysisManagerModuleProxy>(M).getManager();
    for (auto &F : M) {
      if (F.empty()) {
        continue;
      }
      // We need the iterated dominance frontier of defs to place phi-nodes
      DominatorTree &DT = FAM.getResult<DominatorTreeAnalysis>(F);
      ForwardIDFCalculator IDF(DT);
      // Find allocas and then link defs (stores) and uses (loads) to variables
      // (allocas)
      for (auto &InstRef : F.getEntryBlock()) {
        if (auto *Alloca = dyn_cast<AllocaInst>(&InstRef)) {
          auto VarInfo = std::make_unique<VariableInfo>(Alloca);
          if (escapes(Alloca)) continue;
          linkDefsAndUsesToVar(VarInfo.get());
          VariableInfos.push_back(std::move(VarInfo));
        }
      }

      // Insert phi-nodes in iterated dominance frontier of defs
      for (auto &VarInfo : VariableInfos) {
        IDF.setDefiningBlocks(VarInfo->DefBlocks);
        SmallVector<BasicBlock *, 32> PHIBlocks;
        IDF.calculate(PHIBlocks);
        for (auto *PB : PHIBlocks) {
          auto *PN = PHINode::Create(VarInfo->Alloca->getAllocatedType(), 0, "",
                               &PB->front());
          InstToVariableInfo[PN] = VarInfo.get();
        }
      }

      // Do renaming
      DomTreeNode *DN = DT.getNode(&F.getEntryBlock());
      renameRecursive(DN);
      // Remove trash
      for (auto *Trash : TrashList) {
        Trash->eraseFromParent();
      }
      TrashList.clear();
      for (auto &VarInfo : VariableInfos) {
        VarInfo->Alloca->eraseFromParent();
      }
      InstToVariableInfo.clear();
      VariableInfos.clear();
    }
    return PreservedAnalyses::none(); // ? todo check
  };
}; // end struct OurMemToReg
} // end namespace

extern "C" LLVM_ATTRIBUTE_WEAK ::llvm::PassPluginLibraryInfo
llvmGetPassPluginInfo() {
  return {.APIVersion = LLVM_PLUGIN_API_VERSION,
          .PluginName = "Our mem2reg pass",
          .PluginVersion = "v0.1",
          .RegisterPassBuilderCallbacks = [](PassBuilder &PB) {
            PB.registerPipelineStartEPCallback(
                [](ModulePassManager &MPM, OptimizationLevel Level) {
                  MPM.addPass(OurMemToReg());
                });
          }};
}
