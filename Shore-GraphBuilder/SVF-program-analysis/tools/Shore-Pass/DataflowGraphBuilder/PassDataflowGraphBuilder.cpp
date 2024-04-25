//===- DataflowGraphBuilder.cpp
//-------------------------------------------------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file performs analysis of the application to generate data that can
// be used to create a HexBox policy
//
//===----------------------------------------------------------------------===//

#include "llvm/ADT/Statistic.h"
#include "llvm/IR/Module.h"
// #include "llvm/IR/CallSite.h"
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/Module.h>
#include <llvm/Support/raw_ostream.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

#include "MemoryModel/PointerAnalysis.h"
#include "MemoryModel/PointerAnalysisImpl.h"
#include "SVF-FE/LLVMUtil.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "Util/Options.h"
#include "WPA/WPAPass.h"

#include "WPA/Andersen.h" // For points-to analysis, which is often a prerequisite
#include "Graphs/ICFG.h"   // Interprocedural Control-Flow Graph
#include "DDA/DDAPass.h"
#include "CFL/CFLAlias.h"
#include "CFL/CFLVF.h"
#include "Graphs/VFG.h"


#include "llvm/ADT/SmallSet.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DebugInfo.h"
#include "llvm/IR/DebugInfoMetadata.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/InstIterator.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Pass.h"
#include "llvm/Support/Debug.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Transforms/Instrumentation.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"


#include "dependencyWalker.h"

using namespace std;
// using namespace SVF;

namespace {

using namespace llvm;

struct DataflowGraphBuilder : public ModulePass {
    static char ID;
    DataflowGraphBuilder() : ModulePass(ID) {
        // initializeDataflowGraphBuilderPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override {
        errs() << "Hello, I am in DataflowGraphBuilder Pass\n";

        bool is_transformed = false;

        is_transformed |= generateDataflowGraph(M);

        return is_transformed;
    }

    Set<SVF::SVFGNode *> getSVFGNodeUpperDefs(SVFGNode *svfgnode, bool isRecord) {
        //   errs()<<"\nDaaaaDDDD \n";
        
        SVF::Set<SVF::SVFGNode *> upperDefs;
 /* 
        if (isVisit((SVF::SVFGNode *)svfgnode) && isRecord) {
            return upperDefs;
        }
        if (isRecord) {
            visit((SVF::SVFGNode *)svfgnode);
        }
*/
        const SVF::SVFGNode::GEdgeSetTy &edges = svfgnode->getInEdges();
        //    errs()<<edges.size()<<" RCE \n";
        //    const SVFGNode::GEdgeSetTy edges(_edges);
        // errs()<<" ccccbbbbbOURCE \n";
        //   const SVFGNode::GEdgeSetTy _edges=svfgnode->getInEdges();
        errs() << "Number of dependencies : " << edges.size() << "\n";
        for (const auto& edge : edges) {
            //  errs()<<" SOUR\n";
            const SVF::SVFGNode *src_svfnode = edge->getSrcNode();
            upperDefs.insert((SVFGNode *)src_svfnode);
            errs() << " SOURCE vfgnode " << src_svfnode->toString() << "\n";
        }
        //    errs()<<" SENNNOUR\n";
        return upperDefs;
    }

    Set<SVF::VFGNode *> getVFGNodeUpperDefs(VFGNode *vfgnode, bool isRecord) {
        //   errs()<<"\nDaaaaDDDD \n";
        
        SVF::Set<SVF::SVFGNode *> upperDefs;
        const SVF::SVFGNode::GEdgeSetTy &edges = vfgnode->getInEdges();
        errs() << "Number of dependencies : " << edges.size() << "\n";
        for (const auto& edge : edges) {
            //  errs()<<" SOUR\n";
            const SVF::VFGNode *src_vfgnode = edge->getSrcNode();
            upperDefs.insert((SVFGNode *)src_vfgnode);
            errs() << " SOURCE vfgnode " << src_vfgnode->toString() << "\n";
        }

        return upperDefs;
    }

    bool generateDataflowGraph(Module &M) {
        bool ret_transformed = false;

        SVF::SVFModule* svfModule = SVF::LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(M);
        svfModule->buildSymbolTableInfo();

        /// Build Program Assignment Graph (SVFIR)
        SVF::SVFIRBuilder builder;
        SVF::SVFIR* pag = builder.build(svfModule);

        /// Create Andersen's pointer analysis
        SVF::Andersen* ander = SVF::AndersenWaveDiff::createAndersenWaveDiff(pag);

        /// Query aliases
        /// aliasQuery(ander,value1,value2);

        /// Print points-to information
        /// printPts(ander, value1);

        /// Call Graph
        SVF::PTACallGraph* callgraph = ander->getPTACallGraph();

        /// ICFG
        SVF::ICFG* icfg = pag->getICFG();
        icfg->dump("icfg");

        /// Value-Flow Graph (VFG)
        SVF::VFG* vfg = new VFG(callgraph);

/*
        SVF::SVFIRBuilder builder;
        SVF::SVFIR* svfir = builder.build(svfModule);

        SVF::CFLVF* cfl = new SVF::CFLVF(svfir);
        // cfl->analyze();
        cfl->buildSVFGraph();

        // Get Sparse Value Graph
        SVF::SVFG* svfg = cfl->getSvfg();
*/
//        DepWalker depwalker(M, svfModule, svfir, svfg);

        for (Function &F : M) {
            for (BasicBlock &BB : F) {
                for (Instruction &I : BB) {
                    int is_annotated = 0;

                    if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
                        // Find call and invoke instructions
                        Function *called_function;
                        int should_instrument = 0;

                        if (auto *callInst = dyn_cast<CallInst>(&I)) {

                            called_function = callInst->getCalledFunction();
                        } else if (auto *invokeInst =
                                       dyn_cast<InvokeInst>(&I)) {
                            called_function = invokeInst->getCalledFunction();
                        }

                        if (called_function &&
                            called_function->getName().contains("freshness")) {
                            errs() << "Call to 'FRESHNESS' found in "
                                      "function '"
                                   << F.getName() << "'\n";
                            is_annotated = 1;
                        } else if (called_function &&
                                   called_function->getName().contains(
                                       "consistency")) {
                            errs() << "Call to 'CONSISTENCY' found in "
                                      "function '"
                                   << F.getName() << "'\n";
                            is_annotated = 1;
                        } else if (called_function &&
                                   called_function->getName().contains(
                                       "stability")) {
                            errs() << "Call to 'STABILITY' found in "
                                      "function '"
                                   << F.getName() << "'\n";

                            is_annotated = 1;
                        }
                    
                    
                        if (is_annotated) {
                            errs() << "Annotated Instruction: " << I << "\n";
                            // Generate dataflow graph
                            SVF::PAG* pag = vfg->getPAG();
                            if (pag->hasValueNode(&I)) {
                                errs() << "Found Instruction: " << I << " in the SVF graph \n";
                                
                                SVF::NodeID nodeId = pag->getValueNode(&I);
                                SVF::VFGNode* node = vfg->getVFGNode(nodeId);
                                Set<SVF::VFGNode *> defs = getVFGNodeUpperDefs(node, false);
                                for (auto def : defs) {
                                    errs() << "Def: " << def->toString() << "\n";
                                }


                                // Get the points-to information
                                const PointsTo& pts = ander->getPts(nodeId);
                                errs() << "Points-to set size : " << pts.count() << "\n";

                                for (PointsTo::iterator ii = pts.begin(), ie = pts.end();
                                     ii != ie; ii++) {
                                    errs() << " " << *ii << " ";
                                    SVF::PAGNode* targetObj = pag->getGNode(*ii);
                                    if (targetObj->hasValue()) {
                                        errs() << "(" << *targetObj->getValue() << ")\t ";
                                    }
                                }
                            }

                        }
                    }
                }
            }
        }


  //      svfg->dump("svfg");

        return ret_transformed;
    }

    bool generateCallGraph(Module &M) {

        bool ret_transformed = false;

        errs() << "Generating Call Graph\n";

        std::vector<std::string> module_name_vec;
        std::string tmp(M.getModuleIdentifier());
        module_name_vec.push_back(tmp);

        SVF::SVFModule *svfModule =
             SVF::LLVMModuleSet::getLLVMModuleSet()->buildSVFModule(M);
        svfModule->buildSymbolTableInfo();
        errs() << "Get SVF Module\n";


        /// Build Program Assignment Graph (SVFIR)
        SVF::SVFIRBuilder builder;
        SVF::SVFIR* pag = builder.build(svfModule);

        errs() << "Program Dependency Graph is Built\n";

        /// Create Andersen's pointer analysis
        SVF::Andersen* ander = SVF::AndersenWaveDiff::createAndersenWaveDiff(pag);
        /// Query aliases
        /// aliasQuery(ander,value1,value2);

        /// Print points-to information
        /// printPts(ander, value1);

        /// Call Graph
        SVF::PTACallGraph* callgraph = ander->getPTACallGraph();

        // std::string callgraph_filename = "callgraph";

        callgraph->dump("callgraph");


        /// ICFG
        SVF::ICFG* icfg = pag->getICFG();
        icfg->dump("icfg");

        return ret_transformed;
    }
};

}  // namespace

char DataflowGraphBuilder::ID = 0;

static llvm::RegisterPass<DataflowGraphBuilder> X("DataflowGraphBuilder",
                                                "DataflowGraphBuilder Pass",
                                                false /* Only looks at CFG */,
                                                false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) {
        PM.add(new DataflowGraphBuilder());
    });
