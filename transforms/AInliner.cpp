//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.

#include <cassert>
#include <set>

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/Module.h"
#include "llvm/Pass.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Support/raw_os_ostream.h"
#include "llvm/Transforms/Utils/Cloning.h"

llvm::cl::list<std::string> noinline("ainline-noinline",
        llvm::cl::desc("Do not inline the given functions (comma-separated)\n"),
        llvm::cl::CommaSeparated);

using namespace llvm;

class AgressiveInliner : public ModulePass {
public:
  static char ID;

  AgressiveInliner() : ModulePass(ID) {}

  bool runOnModule(Module& M) override;
  bool runOnFunction(Function &F);
};

static RegisterPass<AgressiveInliner>
DLTU("ainline", "Agressive inliner - inline as much as you can.");

char AgressiveInliner::ID;

bool AgressiveInliner::runOnModule(Module& M) {
  bool changed = false;
  for (auto& F : M) {
      changed |= runOnFunction(F);
  }
  return changed;
}

static bool shouldIgnore(const std::string& name) {
    for (const auto& ignore : noinline) {
        if (name == ignore)
            return true;
    }
    return false;
}

bool AgressiveInliner::runOnFunction(Function &F) {
    bool changed = false;
    std::vector<CallInst *> calls;
    for (auto& B : F) {
        for (auto& I : B) {
            if (auto *CI = dyn_cast<CallInst>(&I)) {
                calls.push_back(CI);
            }
        }
    }

    // FIXME: this is really stupid naive way to inline...
    for (auto *CI : calls) {
        //llvm::errs() << "Inlining: " <<*CI << "\n";
        auto *CV = CI->getCalledValue()->stripPointerCasts();
        auto *fun = llvm::dyn_cast<llvm::Function>(CV);
        if (!fun)
            continue; // funptr

        auto name = fun->getName().str();
        if (shouldIgnore(name))
            continue;

        InlineFunctionInfo IFI;
        auto result = InlineFunction(CI, IFI);
        if (!result) {
          //llvm::errs() << "Failed inlining: " << *CI << "\n";
          //llvm::errs() << "  " << static_cast<const char *>(result) << "\n";
        } else {
            changed = true;
        }
    }

    return changed;
}

