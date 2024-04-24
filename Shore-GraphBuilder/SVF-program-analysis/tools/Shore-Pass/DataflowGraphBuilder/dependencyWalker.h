#pragma once 

#pragma once 
#include <cassert>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>
#include <string>
#include <fstream>
#include <fcntl.h>
#include <unistd.h>

#include "SVF-FE/LLVMUtil.h"
#include "WPA/WPAPass.h"
#include "Util/Options.h"
#include "DDA/DDAPass.h"
#include "SVF-FE/SVFIRBuilder.h"
#include "CFL/CFLAlias.h"
#include "CFL/CFLVF.h"

using namespace llvm;
using namespace std;
using namespace SVF;

// class Instrumenter;

class DepWalker{

    // friend DFAnalysisPass;
    public: 
    typedef Set<SVFGNode*> VisitedSVFGNodeSetTy;
    typedef Map<SVFGNode*,int> VisitedSVFGNode2DepNumTy;
    typedef Map<SVFGNode*,Set<SVFGNode*>> VisitedSVFGNode2DepMapTy;
    // typedef Map<SVFGNode*, bool>  VisitedSVFGNodeMap;
    VisitedSVFGNodeSetTy visitedSVFGNodeSet;
    VisitedSVFGNodeSetTy calcedSVFGNodeSet;
    VisitedSVFGNode2DepNumTy visitedSVFGNode2DepNum; 
    VisitedSVFGNode2DepNumTy visitedSVFGNode2LoadNum;
    VisitedSVFGNode2DepNumTy visitedSVFGNode2StoreNum;
    VisitedSVFGNode2DepMapTy visitedSVFGNode2DepMap;
    VisitedSVFGNode2DepMapTy phi2Cdep;
    llvm::Module &M;
    SVFModule* svfModule;
     SVFIR* svfir;
     SymbolTableInfo::ValueToIDMapTy& valSymMap;
    SVFG* svfg;
    SVFIR* pag;
    PTACallGraph* ptacallgraph;
//    Instrumenter * instrumenter;

   
    struct DFAnalysisPass* mainpass; 
    bool saveState;
    bool isControlDep;
    bool isICFGcdep;
    
    DepWalker(llvm::Module &m,SVFModule* _svfModule, SVFIR* _svfir,SymbolTableInfo::ValueToIDMapTy& _valSymMap,SVFG* _svfg, SVFIR* _pag):M(m),svfModule(_svfModule),
    svfir(_svfir),valSymMap(_valSymMap),svfg(_svfg),
    pag(_pag),saveState(false),isControlDep(false),isICFGcdep(false)
    {
            // errs()<<" svfgmo: "<<svfModule<<"\n";
            PointerAnalysis* _pta=svfg->getPTA();
            ptacallgraph=_pta->getPTACallGraph();
    }

        Set<Instruction*> find_difference(Set<BasicBlock*>);
    Set<Instruction *> find_difference_group( std::vector<Set<BasicBlock*>> vecbbset);
    Set<Instruction *> find_inter_pro_difference(Set<BasicBlock *> ,Set<SVFGNode*>,SVFGNode*) ;
    Set<SVFFunction*> getDistinguishFunction(Set<Function*> llvmfset,
  Map< SVFFunction*, Map<unsigned long,Set<CallICFGNode* > > >&fun2equivCallsite,
  Map< SVFFunction*, Map<unsigned long,Set<CallICFGNode* > > >& fun2singleCallsite);
    Instruction* getFunPtr(CallICFGNode* cifgnode);
     typedef Map<BasicBlock*,Set<BasicBlock*>> BB2SetBBMapTy;
        BB2SetBBMapTy bb2ReachableBB;
    Instruction* getBranchInst(llvm::BasicBlock* bb);
    Set<BasicBlock*> getReachableBlocks( BasicBlock *TargetBB);

    inline void visit(SVFGNode* svfgnode){
            visitedSVFGNodeSet.insert(svfgnode);
            
    }
    inline bool isVisit(SVFGNode* svfgnode){
        auto search=visitedSVFGNodeSet.find(svfgnode);
            return search!=visitedSVFGNodeSet.end();
            // visitedSVFGNodeSet.insert(svfgnode);
    }

    inline bool isCalc(SVFGNode* svfgnode){
        auto search=calcedSVFGNodeSet.find(svfgnode);
            return search!=calcedSVFGNodeSet.end();
            // visitedSVFGNodeSet.insert(svfgnode);
    }
    inline void calc(SVFGNode* svfgnode){
            Set<SVFGNode*> emptyset;
            visitedSVFGNode2DepMap.insert(std::make_pair(svfgnode,emptyset));
            
    }
    
    inline bool isAllVisit(Set<SVFGNode*> nodeset){
            for(auto n:nodeset){
                if(!isVisit(n)){
                    return false;
                }
            }
            return true;

    }
    inline Set<SVFGNode*> getNew(Set<SVFGNode*> nodeset){
        Set<SVFGNode*> newnodeset;
        for(auto n:nodeset){
                if(!isVisit(n)){
                    newnodeset.insert(n);
                }
            }
            return newnodeset;
    }
    Set<SVFGNode*> getUpperDefs(Instruction* inst);
    void getUpperStore(SVFGNode *svfgnode);
    void getUpperLoad(SVFGNode *svfgnode);
    Set<SVFGNode*> getUpperDefs(SVFGNode* svfgnode,bool isRecord=true);
    void getAllUpperDeps(SVFGNode* svfgnode);
    void getAllUpperDeps(Instruction* inst);
    void getAllUpperDeps_saveStates(SVFGNode* svfgnode);
    Set<Instruction*> getUpperDefsStrLdr(Instruction* inst);
    

    SVFGNode* getSVFGNodeFromInst(Instruction* inst);

    VisitedSVFGNodeSetTy getAllUpperDeps_single(Instruction* inst,bool track=true);
    inline int getMaxfromMap(VisitedSVFGNode2DepNumTy set2num){
        int max=-1;
        for(auto it:set2num){
            if(max<it.second){
                max=it.second;
            }
        }
        return max;
    }
    inline Set<Instruction*> getLoad(Set<SVFGNode*>& svfgnodes){
        Set<Instruction*> loads;
        for(auto n:svfgnodes){
                const Value* value=n->getValue();
                if(!value){
                    continue;
                }
                const Instruction* inst= llvm::dyn_cast<llvm::Instruction>(value);
                if(inst){
                //    errs()<<"true:"<<*inst<<" \n"; 
                   if(const LoadInst* loadInst= llvm::dyn_cast<llvm::LoadInst>(inst)){
                    loads.insert((Instruction*)inst);
                    // errs()<<"true:"<<*inst<<" \n";
                   }
                }
        }
        return loads;
        
    }
    inline Instruction* getLoad(SVFGNode* n){
                Instruction* load=nullptr;
        
                const Value* value=n->getValue();
                if(!value){
                    return nullptr;
                }
                const Instruction* inst= llvm::dyn_cast<llvm::Instruction>(value);
                if(inst){
                //    errs()<<"true:"<<*inst<<" \n"; 
                   if(const LoadInst* loadInst= llvm::dyn_cast<llvm::LoadInst>(inst)){
                    load=(Instruction*)inst;
                  
                    // errs()<<"true:"<<*inst<<" \n";
                   }
                }
        
        return load;
    }

    inline Instruction* getStore(SVFGNode* n){
                Instruction* store=nullptr;
        
                const Value* value=n->getValue();
                if(!value){
                    return nullptr;
                }
                const Instruction* inst= llvm::dyn_cast<llvm::Instruction>(value);
                if(inst){
                //    errs()<<"true:"<<*inst<<" \n"; 
                   if(const StoreInst* storeInst= llvm::dyn_cast<llvm::StoreInst>(inst)){
                    store=(Instruction*)inst;
                  
                    // errs()<<"true:"<<*inst<<" \n";
                   }
                }
        
        return store;
    }

    inline Set<Instruction*> getStore(Set<SVFGNode*>& svfgnodes){
        Set<Instruction*> stores;
        for(auto n:svfgnodes){
                const Value* value=n->getValue();
                if(!value){
                    continue;
                }
                const Instruction* inst= llvm::dyn_cast<llvm::Instruction>(value);
                if(inst){
                //    errs()<<"true:"<<*inst<<" \n"; 
                   if(const StoreInst* storeInst= llvm::dyn_cast<llvm::StoreInst>(inst)){
                    stores.insert((Instruction*)inst);
                    // errs()<<"true:"<<*inst<<" \n";
                   }
                }
        }
        return stores;
        
    }

    void addControlDependency(SVFGNode* n,Set<SVFGNode*>& upperdefs,Set<SVFGNode *> tmp );
};

