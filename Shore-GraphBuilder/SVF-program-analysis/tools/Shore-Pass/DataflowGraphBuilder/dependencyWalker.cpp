
#include "dependencyWalker.h"

using namespace llvm;
using namespace std;
using namespace SVF;

Set<Instruction *> DepWalker::getUpperDefsStrLdr(Instruction *inst) {}

Set<SVFGNode *> DepWalker::getUpperDefs(SVFGNode *svfgnode, bool isRecord) {
    //   errs()<<"\nDaaaaDDDD \n";
    Set<SVFGNode *> upperDefs;
    if (isVisit((SVFGNode *)svfgnode) && isRecord) {
        return upperDefs;
    }
    if (isRecord) {
        visit((SVFGNode *)svfgnode);
    }

    const SVFGNode::GEdgeSetTy &edges = svfgnode->getInEdges();
    //    errs()<<edges.size()<<" RCE \n";
    //    const SVFGNode::GEdgeSetTy edges(_edges);
    // errs()<<" ccccbbbbbOURCE \n";
    //   const SVFGNode::GEdgeSetTy _edges=svfgnode->getInEdges();
    //    errs()<<edges.size()<<" SOURCE \n";
    for (auto edge : edges) {
        //  errs()<<" SOUR\n";
        const SVFGNode *src_svfnode = edge->getSrcNode();
        // errs()<<" sssSOURCE \n";
        upperDefs.insert((SVFGNode *)src_svfnode);
        errs()<<" SOURCE vfgnode "<<src_svfnode->toString()<<"\n";
    }
    //    errs()<<" SENNNOUR\n";
    return upperDefs;
}

Set<SVFGNode *> DepWalker::getUpperDefs(Instruction *inst) {
    Set<SVFGNode *> upperDefs;
    SVFGNode *svfgnode = getSVFGNodeFromInst(inst);
    if (svfgnode == nullptr) {
        return upperDefs;
    }
    return getUpperDefs((SVFGNode *)svfgnode);
}

void DepWalker::getUpperStore(SVFGNode *svfgnode) {
    Instruction *loadInst = getLoad(svfgnode);
    if (!loadInst) {
        return;
    }

    /*
          if(instrumenter->load2store.find(loadInst)!=instrumenter->load2store.end()){
           return ;
          }

          if(instrumenter->load2numdef.find(loadInst)!=instrumenter->load2numdef.end()){
           return;
          }
   */
    std::stack<SVFGNode *> worklist;
    Set<SVFGNode *> visited;
    int num_stores = 0;
    worklist.push(svfgnode);
    Set<Instruction *> upperdefinst;
    while (worklist.size() > 0) {
        SVFGNode *n = worklist.top();
        worklist.pop();
        visited.insert(n);
        Set<SVFGNode *> tmp = getUpperDefs(n, false);
        for (auto m : tmp) {
            Instruction *storeInst = getStore(m);
            if (!storeInst && (visited.find(m) == visited.end())) {
                worklist.push(m);
            } else if (storeInst && ((visited.find(m) == visited.end()))) {
                num_stores++;
                upperdefinst.insert(storeInst);
            }
        }
    }
    //      instrumenter->load2numdef.insert(std::make_pair(loadInst,upperdefinst.size()));
    //      instrumenter->load2store.insert(std::make_pair(loadInst,upperdefinst));
    // errs()<<"loadInst: "<<*loadInst<<"\n";
    for (auto i : upperdefinst) {
        // errs()<<" ---- "<<*i<<"\n";
    }
}

void DepWalker::getUpperLoad(SVFGNode *svfgnode) {
    Instruction *storeInst = getStore(svfgnode);
    if (!storeInst) {
        return;
    }
    /*     if(instrumenter->store2load.find(storeInst)!=instrumenter->store2load.end()){
           return ;
          }
   */
    //  if(instrumenter->load2numdef.find(loadInst)!=instrumenter->load2numdef.end()){
    //   return;
    //  }

    std::stack<SVFGNode *> worklist;
    Set<SVFGNode *> visited;
    // int num_stores=0;
    worklist.push(svfgnode);
    Set<Instruction *> upperdefinst;
    while (worklist.size() > 0) {
        SVFGNode *n = worklist.top();
        worklist.pop();
        visited.insert(n);
        Set<SVFGNode *> tmp = getUpperDefs(n, false);
        for (auto m : tmp) {
            Instruction *loadInst = getLoad(m);
            if (!loadInst && (visited.find(m) == visited.end())) {
                worklist.push(m);
            } else if (loadInst && ((visited.find(m) == visited.end()))) {
                // num_stores++;
                upperdefinst.insert(loadInst);
            }
        }
    }
    //   instrumenter->store2load.insert(std::make_pair(storeInst,upperdefinst));
    // instrumenter->load2numdef.insert(std::make_pair(loadInst,upperdefinst.size()));
    // errs()<<"storeInst: "<<*storeInst<<"\n";
    for (auto i : upperdefinst) {
        // errs()<<" ----~!!! "<<*i<<"\n";
    }
}

void DepWalker::getAllUpperDeps(SVFGNode *svfgnode) {
    // vector<SVFGNode*> allnodes;
    Set<SVFGNode *> upperdefs = getUpperDefs(svfgnode);
    /*
      if(instrumenter->maintainDef_num){
          getUpperStore(svfgnode);
        }
        if(instrumenter->maintainDef_num){
          getUpperLoad(svfgnode);
        }
     */

    while (!isAllVisit(upperdefs)) {
        Set<SVFGNode *> unvisited = getNew(upperdefs);
        upperdefs.clear();
        //  Set<SVFGNode*> newupper;
        for (auto n : unvisited) {
            Set<SVFGNode *> tmp = getUpperDefs(n);
            for (auto i : tmp) {
                upperdefs.insert(i);
                /*
                          if(instrumenter->maintainDef_num){
                            getUpperStore(i);
                          }
                */
            }
            // processing phi instruction
            if (isControlDep) {
                // errs()<<"  hhhh1\n";
                addControlDependency(n, upperdefs, tmp);
                // errs()<<"  sadfasdfads\n";
            }
        }
    }
}

void DepWalker::addControlDependency(SVFGNode *n, Set<SVFGNode *> &upperdefs,
                                     Set<SVFGNode *> tmp) {
    if (MSSAPHISVFGNode::classof(n) || PHISVFGNode::classof(n) ||
        FormalINSVFGNode::classof(n) || ActualOUTSVFGNode::classof(n) ||
        FormalParmVFGNode::classof(n)) {
        // errs() << "\nthis is MSSAPHISVFGNode " << n->toString() << "\n";
        Set<SVFGNode *> svfgnodeCdeps;
        if (phi2Cdep.find(n) != phi2Cdep.end()) {
            svfgnodeCdeps = phi2Cdep[n];
        } else {
            // Set<Instruction *> converted;
            Set<BasicBlock *> converted;
            Set<SVFGNode *> origins;
            // errs()<<" size of phi:"<<tmp.size()<<"\n";
            for (auto m : tmp) {
                if (m == nullptr) {
                    // errs() << "NULL_Occur\n";
                    continue;
                }

                // const Value *value = m->getValue();
                const ICFGNode *icfgnode = m->getICFGNode();
                const BasicBlock *nodebb = icfgnode->getBB();
                // if (!value) {
                //   // Set<SVFGNode*> upp=getUpperDefs(m,false);

                //   errs()<<" SVFGNode: "<<m->toString()<<" ;; ICFGNnode:
                //   "<<icfgnode->toString()<<" ;;BB: "<<"\n";
                //    nodebb->printAsOperand(errs(),false);
                //    errs() << "none_ value\n";
                //   continue;
                // }

                // const Instruction *inst =
                // llvm::dyn_cast<llvm::Instruction>(value);
                if (nodebb) {
                    //  errs()<<"---- "<<m->toString()<<"\n";
                    origins.insert(m);
                    converted.insert((BasicBlock *)nodebb);
                }
            }
            // errs()<<" add cdep for "<<n->toString
            Set<Instruction *> cdeps;
            if (IntraMSSAPHISVFGNode::classof(n) ||
                IntraPHISVFGNode::classof(n)) {
                cdeps = find_difference(converted);
                // cdeps=mainpass->_find_difference(converted);
            } else {
                errs() << "Upper: " << n->toString() << "\n";
                if (isICFGcdep) {
                    // cdeps=find_difference(converted);
                    // errs()<<"why here\n";

                    for (auto p : origins) {
                        // errs()<<"-----"<<p->toString()<<"\n";
                    }
                    cdeps = find_inter_pro_difference(converted, origins, n);
                    // errs()<<"fcdfer here\n";
                } else {
                    // cdeps=mainpass->_find_difference(converted);
                    cdeps = find_difference(converted);
                }
            }

            for (auto tcdep : cdeps) {  // convert instruction to svfgnode;

                SVFGNode *ttmp = getSVFGNodeFromInst(tcdep);

                if (ttmp) {
                    svfgnodeCdeps.insert(ttmp);
                } else {
                }
            }
            phi2Cdep.insert(std::make_pair(n, svfgnodeCdeps));

            //
        }

        for (auto toAddSvfgnode : svfgnodeCdeps) {
            upperdefs.insert(toAddSvfgnode);
        }

        // errs()<<"ddd\n";
        // std::vector<Instruction*> vc(converted.begin(),converted.end());
        // errs()<<*vc[0]<<"ccvv\n";
    }
}
void DepWalker::getAllUpperDeps_saveStates(SVFGNode *svfgnode) {
    // vector<SVFGNode*> allnodes;
    if (isCalc(svfgnode)) {
        return;
    }
    calc(svfgnode);

    // Set<SVFGNode*> alldeps;

    Set<SVFGNode *> upperdefs = getUpperDefs(svfgnode);

    for (auto n : upperdefs) {
        visitedSVFGNode2DepMap[svfgnode].insert(n);
    }
    for (auto n : upperdefs) {
        getAllUpperDeps_saveStates(n);
        for (auto m : visitedSVFGNode2DepMap[n]) {
            visitedSVFGNode2DepMap[svfgnode].insert(m);
        }
    }
}

void DepWalker::getAllUpperDeps(Instruction *inst) {
    // errs()<<" why "<<*inst;
    // SVFGNode* _svfgnode=getSVFGNodeFromInst(inst);
    // errs()<<" aaaawhy "<<_svfgnode;
    SVFGNode *svfgnode = getSVFGNodeFromInst(inst);
    if (svfgnode == nullptr) {
        return;
    }
    // errs()<<" wwwwhy "<<svfgnode<<"\n";
    // errs()<<"mmmmm \n";
    // errs()<<svfgnode->toString()<<"\n";
    if (saveState) {
        getAllUpperDeps_saveStates(svfgnode);
    } else {
        getAllUpperDeps(svfgnode);
    }
}

SVFGNode *DepWalker::getSVFGNodeFromInst(Instruction *inst) {
    if (!inst) {
        return nullptr;
    }
    // Set<SVFGNode*> upperDefs;
    SymbolTableInfo::ValueToIDMapTy::const_iterator iter = valSymMap.find(inst);
    Function *llvmfun = inst->getFunction();
    const SVFFunction *svfFun = svfModule->getSVFFunction(llvmfun);
    SVFVar *node = pag->getGNode(iter->second);

    if (iter == valSymMap.end()) {
        // errs() << "NOTfound : " << *inst << "\n";
        return nullptr;
    }

    const SVFGNode *svfgnode = nullptr;
    if (svfg->hasDefSVFGNode(node)) {
        svfgnode = svfg->getDefSVFGNode(node);
        //  errs()<<" here is vfgnode "<<svfgnode->toString()<<"\n";
    } else {
        //   errs()<<" NOOOOO vfgnode "<<node->toString()<<"\n";

        const IRGraph::SVFStmtSet &svfStmtSet = pag->getValueEdges(inst);
        //  errs()<<" SSSNOOOode "<<"\n";
        //  for(IRGraph::SVFStmtSet::const_iterator
        //  _it=svfStmtSet.begin();_it!=svfStmtSet.end();_it++){
        for (auto it : svfStmtSet) {
            // errs()<<"   vvvv  "<<it->toString()<<"   cc\n";
            if (const ReturnInst *retInst =
                    SVFUtil::dyn_cast<ReturnInst>(inst)) {
                //  const SVFFunction *svfFun = &fun;
                const PAGNode *fun_return = pag->getFunRet(svfFun);
                svfgnode = svfg->getFormalRetVFGNode(fun_return);
                //  errs()<<" Return vfgnode "<<svfgnode->toString()<<"\n";
            } else {
                // errs() << "\n others " << it->toString();
                if (svfg->hasStmtVFGNode(it)) {
                    svfgnode = svfg->getStmtVFGNode(it);
                }
                //   errs()<<" KKKOOO vfgnode "<<svfgnode->toString()<<"\n";
            }
        }
        // errs()<<" qqqqqSSSNOOOode "<<"\n";
    }
    if (svfgnode == nullptr) {
        // errs() << " this is null_svfg_node_ptr: " << *inst << "  "
        //        << "\n";
    }
    return (SVFGNode *)svfgnode;
}
// get the all depedency for a instruction
DepWalker::VisitedSVFGNodeSetTy DepWalker::getAllUpperDeps_single(
    Instruction *inst, bool track) {
    VisitedSVFGNodeSetTy tmp = visitedSVFGNodeSet;
    //   errs()<<"````````size : "<<visitedSVFGNodeSet.size()<<"\n";
    visitedSVFGNodeSet.clear();

    getAllUpperDeps(inst);

    VisitedSVFGNodeSetTy result = visitedSVFGNodeSet;
    visitedSVFGNodeSet = tmp;
    if (track) {
        for (auto n : result) {
            visitedSVFGNodeSet.insert(n);
        }
    }
    {
        SVFGNode *svfgnode = getSVFGNodeFromInst(inst);
        visitedSVFGNode2DepNum.insert(std::make_pair(svfgnode, result.size()));
        visitedSVFGNode2LoadNum.insert(
            std::make_pair(svfgnode, getLoad(result).size()));
        visitedSVFGNode2StoreNum.insert(
            std::make_pair(svfgnode, getStore(result).size()));
    }

    //  errs()<<"size : "<<visitedSVFGNodeSet.size()<<"\n";

    return result;
}

Set<BasicBlock *> DepWalker::getReachableBlocks(BasicBlock *TargetBB) {
    Set<BasicBlock *> ReachingBlocks;
    ReachingBlocks.insert(TargetBB);

    // Perform backwards traversal of the dominator tree
    std::vector<BasicBlock *> WorkList;
    WorkList.push_back(TargetBB);
    while (!WorkList.empty()) {
        BasicBlock *BB = WorkList.back();
        WorkList.pop_back();
        for (auto *Pred : predecessors(BB)) {
            if (ReachingBlocks.insert(Pred).second) {
                WorkList.push_back(Pred);
            }
        }
    }
    return ReachingBlocks;
}

Instruction *DepWalker::getFunPtr(CallICFGNode *cifgnode) {
    if (!cifgnode) {
        errs() << "null cfignoe\n";
        return nullptr;
    }
    // errs()<<" hhhhsdsdxw ";
    errs() << cifgnode->toString() << "\n";
    const Instruction *cinst = cifgnode->getCallSite();

    //  errs()<<" kojhioyjo\n";
    const CallInst *csInst = SVFUtil::dyn_cast<CallInst>(cinst);
    if (!csInst) {
        errs() << " konullcsInt\n";
        return nullptr;
    }
    // errs()<<" asxrr\n"<<csInst;
    // errs()<<"\n  dsddd "<<cifgnode->toString()<<"ss";
    Function *callee = csInst->getCalledFunction();
    // errs()<<" bnbmnbbjk\n";
    if (!callee) {
        // errs()<<" ncheuyce\n";
        Value *calledValue = csInst->getCalledOperand();
        // errs()<<" ijoikopkop\n";
        const Instruction *toadd = SVFUtil::dyn_cast<Instruction>(calledValue);
        // results.insert((Instruction *)toadd);
        // errs()<<"this is the func pointer ";
        // errs()<<*calledValue<<"\n";
        return (Instruction *)toadd;
    }
    // errs()<<" trtgbtrbt\n";
    return nullptr;
}

Set<SVFFunction *> DepWalker::getDistinguishFunction(
    Set<Function *> llvmfset,
    Map<SVFFunction *, Map<unsigned long, Set<CallICFGNode *>>>
        &fun2equivCallsite,
    Map<SVFFunction *, Map<unsigned long, Set<CallICFGNode *>>>
        &fun2singleCallsite) {
    // Map<Function*,Set<Instruction*> > results;
    Map<SVFFunction *, Set<PTACallGraphEdge *>> ret_results;

    Map<SVFFunction *, Set<SVFFunction *>> fun2ReachFunmap;
    Set<SVFFunction *> funs2analysis;
    Set<SVFFunction *> fset;
    for (auto item : llvmfset) {  // first initialize the reacahble func map
        Function *fun = item;
        // errs()<<"dsvds\n"<<svfModule<<"  d\n";
        // svfModule->getSVFFunction(fun);
        // errs()<<"vcdfvdf"<<fun->getName()<<"\n";
        SVFFunction *svffun = (SVFFunction *)svfModule->getSVFFunction(fun);
        fset.insert(svffun);
        // errs()<<"cdssd\n";
        Set<SVFFunction *> tmp = ptacallgraph->getReachableFunctions(svffun);
        // errs()<<"cdwc\n";
        fun2ReachFunmap.insert(std::make_pair(svffun, tmp));
        for (auto kk : tmp) {
            funs2analysis.insert(kk);
            if (!kk) {
                // errs()<<"nnnnn\n";
            }
            // errs()<<"Here is rechacble "<<kk->getLLVMFun()->getName()<<"\n";
        }
    }
    errs() << "RSize: " << funs2analysis.size() << "\n";
    // for()

    Set<SVFFunction *> criticalFunc;
    // Map<SVFFunction* , Map<SVFFunction*, PTACallGraphEdge* > >
    // fun2suc2calledge;
    Map<SVFFunction *, Set<PTACallGraphEdge *>> fun2calledges;
    Map<PTACallGraphEdge *, unsigned long> edge2ReachMap;
    Map<SVFFunction *, unsigned long> func2ReachMap;
    // errs()<<"analyzie_reachable\n";
    // Map<SVFFunction* , Set<PTACallGraphEdge* >> criti_calledges;
    for (auto sfun : funs2analysis) {  // initialize the reachstate of each fun,
                                       // only analysis the rechable function
        // errs()<<"now for: "<<sfun->getLLVMFun()->getName()<<"\n";
        Map<SVFFunction *, bool> fun2bool;
        unsigned long fRem = 0;
        for (auto ib : fset) {
            bool isReb =
                fun2ReachFunmap[ib].find(sfun) != fun2ReachFunmap[ib].end();
            fun2bool.insert(std::make_pair(ib, isReb));
            fRem += isReb;
            fRem = fRem << 1;
        }
        func2ReachMap.insert(std::make_pair(sfun, fRem));

        Set<SVFFunction *> notAllnullSucc;  // remove all none succ

        Map<SVFFunction *, Map<SVFFunction *, bool>> sun2_fset2bools;
        // errs()<<"fewfew here\n";
        //  Map<SVFFunction*, PTACallGraphEdge* >succ2edge;
        Set<PTACallGraphEdge *> calledges;
        //  errs()<<"wvwwvw\n";
        for (PTACallGraph::CallGraphEdgeConstIter
                 it = ptacallgraph->getCallGraphNode(sfun)->OutEdgeBegin(),
                 eit = ptacallgraph->getCallGraphNode(sfun)->OutEdgeEnd();
             it != eit; ++it) {
            PTACallGraphEdge *edge = *it;
            // errs()<<"dcfv\n";
            SVFFunction *succ =
                (SVFFunction *)edge->getDstNode()->getFunction();
            Map<SVFFunction *, bool> suc_fun2bool;
            bool notallnull = false;
            unsigned long reachMap = 0;
            //  errs()<<"grtgt here\n";
            for (auto ib : fset) {
                bool isReb =
                    fun2ReachFunmap[ib].find(succ) != fun2ReachFunmap[ib].end();
                suc_fun2bool.insert(std::make_pair(ib, isReb));
                reachMap += isReb;
                reachMap = reachMap << 1;
                notallnull = notallnull || isReb;
            }
            // errs()<<" succ: "<<succ->getLLVMFun()->getName()<<" reachmap
            // :"<<reachMap<<"\n";
            //  errs()<<"fegtrtre here\n";
            if (func2ReachMap.find(succ) == func2ReachMap.end()) {
                func2ReachMap.insert(std::make_pair(succ, reachMap));
            }
            //  errs()<<"jiujki7u here\n";
            edge2ReachMap.insert(std::make_pair(edge, reachMap));
            if (notallnull) {
                notAllnullSucc.insert(succ);
                // succ2edge.insert(std::make_pair(succ,edge));
                calledges.insert(edge);
            }
            //  errs()<<"j7uyhrtert here\n";
            sun2_fset2bools.insert(std::make_pair(succ, suc_fun2bool));
        }
        // errs()<<"gyy4re here\n";
        fun2calledges.insert(std::make_pair(sfun, calledges));
        bool dif_master = false;
        bool dif = false;
        // errs()<<"all null succ: "<<notAllnullSucc.size()<<"\n";
        for (auto succ :
             notAllnullSucc) {  // check whether distinguishable func calls.
            int last_tmp = -1;
            // errs()<<"j6765ew here\n";
            // Map<BasicBlock*,bool> suc_bb2bool;
            for (auto ib : fset) {
                //  errs()<<"gt54tefw here\n";
                dif_master =
                    dif_master || fun2bool[ib] != sun2_fset2bools[succ][ib];
                if (last_tmp == -1) {
                    last_tmp = sun2_fset2bools[succ][ib];
                } else {
                    dif = dif || (last_tmp != sun2_fset2bools[succ][ib]);
                    last_tmp = sun2_fset2bools[succ][ib];
                }
            }
            //  errs()<<"uj5654 here\n";
            if (dif_master && dif) {
                break;
            }
        }
        //  errs()<<"vrth56h45e here\n";
        if (dif && dif_master) {
            criticalFunc.insert(sfun);

            ret_results.insert(std::make_pair(sfun, fun2calledges[sfun]));
            //  errs()<<"jikioyhrt here\n";

            // errs() << " function_critical:
            // "<<sfun->getLLVMFun()->getName()<<"\n";
        }
        //  errs()<<"hjy5645x here\n";
    }
    // errs()<<"arrive here\n";
    // filter equivlant edges

    // errs()<<"fdnsjknk here\n";
    for (auto cf : criticalFunc) {
        Set<PTACallGraphEdge *> setedges = fun2calledges[cf];
        Map<unsigned long, Set<PTACallGraphEdge *>> equivalanceedges;
        for (auto e : setedges) {
            unsigned long rem = edge2ReachMap[e];
            if (equivalanceedges.find(rem) == equivalanceedges.end()) {
                Set<PTACallGraphEdge *> eq;
                eq.insert(e);
                equivalanceedges.insert(std::make_pair(rem, eq));

            } else {
                equivalanceedges[rem].insert(e);
            }
            //  Set<const CallICFGNode*> dcs=e->getDirectCalls();
            //  Set<const CallICFGNode*> indcs=e->getIndirectCalls();
        }

        Map<unsigned long, Set<CallICFGNode *>> equivCallsite;
        Map<unsigned long, Set<CallICFGNode *>> singleCallsite;
        for (auto eqedge : equivalanceedges) {
            unsigned long rem = eqedge.first;
            Set<CallICFGNode *> eq_calls;
            Set<CallICFGNode *> to_p_indi_calls;

            for (auto edge : eqedge.second) {
                Set<const CallICFGNode *> dcs = edge->getDirectCalls();
                for (auto tmp : dcs) {
                    eq_calls.insert((CallICFGNode *)tmp);
                }
                Set<const CallICFGNode *> idcs = edge->getIndirectCalls();
                for (auto tmp : idcs) {
                    Set<const SVFFunction *> callees;
                    ptacallgraph->getCallees(tmp, callees);
                    int notnull = 0;
                    for (auto callee : callees) {
                        if (func2ReachMap[(SVFFunction *)callee] != 0) {
                            notnull++;
                            if (func2ReachMap[(SVFFunction *)callee] != rem) {
                                to_p_indi_calls.insert((CallICFGNode *)tmp);
                                notnull = -1;
                                break;
                            }
                        }
                    }
                    if (notnull >= 1) {
                        eq_calls.insert((CallICFGNode *)tmp);
                    }
                }
            }
            equivCallsite.insert(std::make_pair(rem, eq_calls));
            singleCallsite.insert(std::make_pair(rem, to_p_indi_calls));
        }

        fun2equivCallsite.insert(std::make_pair(cf, equivCallsite));
        fun2singleCallsite.insert(std::make_pair(cf, singleCallsite));
    }
    /*
    for(auto cf:criticalFunc){

      Set<Instruction*> callsites;
      Set<PTACallGraphEdge* > setedges=fun2calledges[cf];
      for(auto e : setedges){
             Set<const CallICFGNode*> dcs=e->getDirectCalls();
             for(auto dc:dcs){
               const Instruction* cinst=dc->getCallSite();
               callsites.insert((Instruction*)cinst);
             }

             Set<const CallICFGNode*> indcs=e->getIndirectCalls();
             for(auto indc:indcs){
               const Instruction* cinst=indc->getCallSite();
               callsites.insert((Instruction*)cinst);
             }
      }
      results.insert(std::make_pair(cf,callsites));
    }
    */
    // for(auto)
    // todo --  get a mapping from func to callsites instruction;
    errs() << "CSize: " << criticalFunc.size() << "\n";
    return criticalFunc;
}

Set<Instruction *> DepWalker::find_inter_pro_difference(Set<BasicBlock *> bbset,
                                                        Set<SVFGNode *> origins,
                                                        SVFGNode *n) {
    // PTACallGraph* ptacallgraph=getPTACallGraph();

    //  if(FormalINSVFGNode::classof(n)
    // ||ActualOUTSVFGNode::classof(n)||FormalParmVFGNode::classof(n)){

    // }

    Set<Instruction *> results;
    if (origins.size() <= 1) {
        return results;
    }
    // errs()<<"thhhhhh\n";

    if (ActualOUTSVFGNode::classof(n)) {
        const ActualOUTSVFGNode *ao = SVFUtil::dyn_cast<ActualOUTSVFGNode>(n);
        const CallICFGNode *cifgnode = ao->getCallSite();
        // errs()<<"failtogetp\n";
        Instruction *toadd = getFunPtr((CallICFGNode *)cifgnode);
        // errs()<<"cwcew\n";
        if (toadd) {
            results.insert((Instruction *)toadd);
        }

        return results;
    }

    //  errs()<<"vdfsvdfbgvdfj\n";

    Map<Function *, Set<BasicBlock *>> fun2BBmap;
    Set<Function *> fset;
    for (auto i : bbset) {
        if (!i) {
            // errs() << "nooon " << "\n";
            bbset.erase(i);
            continue;
        }
        //  errs() << "sssfsdno\n";
        // errs()<<"ascfdsfgv\n";
        if (i->getParent()) {
            // errs() << "TODO iCFG tree: " << *i << "\n";
            Function *fun = i->getParent();
            // errs()<<"functName: "<<fun->getName()<<"\n";
            if (fun2BBmap.find(fun) == fun2BBmap.end()) {
                Set<BasicBlock *> tmps;
                tmps.insert(i);
                fun2BBmap.insert(std::make_pair(fun, tmps));
            } else {
                fun2BBmap[fun].insert(i);
            }
            fset.insert(fun);
            // return difference;
        }
        // errs()<<"fwefr\n";
    }

    //  const ICFGNode* icfgnode=m->getICFGNode();
    //           const BasicBlock* nodebb=icfgnode->getBB();
    const Function *selffun = n->getICFGNode()->getFun()->getLLVMFun();
    fset.insert((Function *)selffun);
    if (fun2BBmap.find((Function *)selffun) == fun2BBmap.end()) {
        Set<BasicBlock *> empty;
        fun2BBmap.insert(std::make_pair((Function *)selffun, empty));
    }
    // get determine function;
    //  errs()<<"gtrghrt\n";
    Map<SVFFunction *, Map<unsigned long, Set<CallICFGNode *>>>
        fun2equivCallsite;
    Map<SVFFunction *, Map<unsigned long, Set<CallICFGNode *>>>
        fun2singleCallsite;
    Set<SVFFunction *> criticalFunctions =
        getDistinguishFunction(fset, fun2equivCallsite, fun2singleCallsite);
    // errs()<<"print critical func :"<<"\n";
    for (auto sgg : criticalFunctions) {
        //  errs()<<"dddddd"<<sgg->getLLVMFun()->getName()<<"\n";
    }
    // for individual function:

    for (auto cf : criticalFunctions) {
        Map<unsigned long, Set<CallICFGNode *>> equivCallsite =
            fun2equivCallsite[cf];
        Map<unsigned long, Set<CallICFGNode *>> singleCallsite =
            fun2singleCallsite[cf];

        std::vector<Set<BasicBlock *>> vecbbset;

        for (auto callset : equivCallsite) {
            Set<BasicBlock *> bbset;

            unsigned long rem = callset.first;

            for (auto callsite : callset.second) {
                const Instruction *cinst = callsite->getCallSite();
                if (cinst) {
                    const BasicBlock *bb = cinst->getParent();
                    if (bb) {
                        bbset.insert((BasicBlock *)bb);
                    } else {
                        errs() << "non_bb\n";
                    }
                } else {
                    errs() << "non_cinst\n";
                }
            }
            vecbbset.push_back(bbset);
        }

        for (auto singlecall : singleCallsite) {
            for (auto ind_c : singlecall.second) {
                Set<BasicBlock *> bbset;
                const Instruction *cinst = ind_c->getCallSite();
                if (cinst) {
                    const BasicBlock *bb = cinst->getParent();
                    if (bb) {
                        bbset.insert((BasicBlock *)bb);
                        vecbbset.push_back(
                            bbset);  // bbset only has one element
                        Instruction *toadd = getFunPtr(
                            (CallICFGNode *)
                                ind_c);  // add func pointer to protection.
                        if (toadd) {
                            results.insert(toadd);
                        } else {
                            errs() << "non_func_ptr\n";
                        }

                    } else {
                        errs() << "non_bb\n";
                    }
                } else {
                    errs() << "non_cinst\n";
                }
            }
        }

        Set<Instruction *> toaddInst = find_difference_group(vecbbset);
        for (auto inst : toaddInst) {
            results.insert(inst);
        }
        // todo get control depedency
    }
    // errs()<<"efwerg\n";
    // for(auto item: togetFun){

    //   // Set<>

    // }
    errs() << "!---- added_inter-control_dep: " << results.size() << " ";
    for (auto i : results) {
        //  errs()<<"addingcc:  "<<*i<<"\n";
    }

    Set<Instruction *> single;
    for (auto f : fset) {
        Set<BasicBlock *> individual = fun2BBmap[f];
        Set<Instruction *> toadded = find_difference(individual);
        for (auto tmp : toadded) {
            results.insert(tmp);
            single.insert(tmp);
            // errs()<<"MMMMM "<<*tmp<<"\n";
        }
    }
    errs() << "intra-control_dep: " << single.size()
           << " total_control_dep: " << results.size() << "\n";

    // errs()<<"added control_dep: "<<results.size()<<"\n";
    for (auto i : results) {
        //  errs()<<"total results:  "<<*i<<"\n";
    }
    return results;
}
// group rechable set together
Set<Instruction *> DepWalker::find_difference_group(
    std::vector<Set<BasicBlock *>> vecbbset) {
    Set<Instruction *> difference;

    if (vecbbset.size() <= 1) {
        return difference;
    }

    // Set<BasicBlock *> bset;
    llvm::Function *fun = nullptr;
    for (int i = 0; i < vecbbset.size(); i++) {
        for (auto bb : vecbbset[i]) {
            if (!bb) {
                vecbbset[i].erase(bb);
                continue;
            }
            fun = bb->getParent();
        }
    }

    std::vector<Set<BasicBlock *>> reachablebbset;

    for (int i = 0; i < vecbbset.size(); i++) {
        Set<BasicBlock *> allReachable;
        for (auto bb : vecbbset[i]) {
            if (bb2ReachableBB.find(bb) == bb2ReachableBB.end()) {
                // errs()<<"compute reachable set\n";
                Set<BasicBlock *> tmpset = getReachableBlocks(bb);
                bb2ReachableBB.insert(std::make_pair(bb, tmpset));
            }
            Set<BasicBlock *> reachable = bb2ReachableBB[bb];
            for (auto adding : reachable) {
                allReachable.insert(adding);
            }
        }
        reachablebbset.push_back(allReachable);
    }

    Set<BasicBlock *> difbb;

    for (Function::iterator bb = fun->begin(), e = fun->end(); bb != e; ++bb) {
        BasicBlock *currBB = &(*bb);

        Map<int, bool> bb2bool;
        bool has_effect = false;
        for (int i = 0; i < vecbbset.size(); i++) {
            bool isReb =
                reachablebbset[i].find(currBB) != reachablebbset[i].end();
            bb2bool.insert(std::make_pair(i, isReb));
            has_effect = has_effect || isReb;
        }
        if (!has_effect) {
            continue;
        }
        bool dif_master = false;
        bool dif = false;
        // Map<BasicBlock*,Map<BasicBlock*,bool> > all_suc_bb2bool;
        bool notwholenull = false;
        for (auto succ : successors(currBB)) {
            Map<int, bool> suc_bb2bool;
            int last_tmp = -1;
            notwholenull = false;
            for (int i = 0; i < vecbbset.size(); i++) {
                bool isReb =
                    reachablebbset[i].find(succ) != reachablebbset[i].end();
                suc_bb2bool.insert(std::make_pair(i, isReb));
                dif_master = dif_master || (isReb != bb2bool[i]);
                notwholenull = notwholenull || isReb;
                if (last_tmp == -1) {
                    last_tmp = isReb;
                } else {
                    dif = dif || (last_tmp != isReb);
                    last_tmp = isReb;
                }
            }

            if (!notwholenull) {
                break;
            }
            // notwholenull=false;
            if (dif_master && dif) {
                break;
            }
            // all_suc_bb2bool.insert(std::make_pair(succ,suc_bb2bool));
        }

        if (dif && dif_master && notwholenull) {
            //  errs() << " to difference: \n";
            difbb.insert(currBB);

            // currBB->printAsOperand(errs(), false);
            // errs()<<">>>\n";
            Instruction *tmp = getBranchInst(currBB);

            if (tmp) {
                BranchInst *branchInst = dyn_cast<BranchInst>(tmp);
                llvm::Value *cond = branchInst->getCondition();

                // while(!cond){

                // }

                // errs() << "get terminator : " << *cond << "\n";
                if (Instruction *cdep = dyn_cast<Instruction>(cond)) {
                    difference.insert(cdep);
                }
            }
        }
    }

    // }
    return difference;
}

Set<Instruction *> DepWalker::find_difference(Set<BasicBlock *> bset) {
    // each inst must be not null_ptr
    //  DependenceAnalysis DA;
    //  Function* F=svfgnode->getFun()->getLLVMFun();

    Set<Instruction *> difference;

    if (bset.size() <= 1) {
        return difference;
    }

    // Set<BasicBlock *> bset;
    llvm::Function *fun = nullptr;
    for (auto i : bset) {
        if (!i) {
            // errs() << "nooon " << "\n";
            bset.erase(i);
            continue;
        }
        //  errs() << "sssfsdno\n";

        fun = i->getParent();
        // bset.insert(i->getParent());
    }

    for (auto bb : bset) {
        if (bb2ReachableBB.find(bb) == bb2ReachableBB.end()) {
            // errs()<<"compute reachable set\n";
            Set<BasicBlock *> tmpset = getReachableBlocks(bb);
            bb2ReachableBB.insert(std::make_pair(bb, tmpset));
        }
    }

    Set<BasicBlock *> difbb;

    for (Function::iterator bb = fun->begin(), e = fun->end(); bb != e; ++bb) {
        BasicBlock *currBB = &(*bb);

        Map<BasicBlock *, bool> bb2bool;
        bool has_effect = false;
        for (auto ib : bset) {
            bool isReb =
                bb2ReachableBB[ib].find(currBB) != bb2ReachableBB[ib].end();
            bb2bool.insert(std::make_pair(ib, isReb));
            has_effect = has_effect || isReb;
        }
        if (!has_effect) {
            continue;
        }
        bool dif_master = false;
        bool dif = false;
        // Map<BasicBlock*,Map<BasicBlock*,bool> > all_suc_bb2bool;
        bool notwholenull = false;
        for (auto succ : successors(currBB)) {
            Map<BasicBlock *, bool> suc_bb2bool;
            int last_tmp = -1;
            notwholenull = false;
            for (auto ib : bset) {
                bool isReb =
                    bb2ReachableBB[ib].find(succ) != bb2ReachableBB[ib].end();
                suc_bb2bool.insert(std::make_pair(ib, isReb));
                dif_master = dif_master || (isReb != bb2bool[ib]);
                notwholenull = notwholenull || isReb;
                if (last_tmp == -1) {
                    last_tmp = isReb;
                } else {
                    dif = dif || (last_tmp != isReb);
                    last_tmp = isReb;
                }
            }

            if (!notwholenull) {
                break;
            }
            // notwholenull=false;
            if (dif_master && dif) {
                break;
            }
            // all_suc_bb2bool.insert(std::make_pair(succ,suc_bb2bool));
        }

        if (dif && dif_master && notwholenull) {
            //  errs() << " to difference: \n";
            difbb.insert(currBB);

            // currBB->printAsOperand(errs(), false);
            // errs()<<">>>\n";
            Instruction *tmp = getBranchInst(currBB);

            if (tmp) {
                BranchInst *branchInst = dyn_cast<BranchInst>(tmp);
                llvm::Value *cond = branchInst->getCondition();

                // while(!cond){

                // }

                // errs() << "get terminator : " << *cond << "\n";
                if (Instruction *cdep = dyn_cast<Instruction>(cond)) {
                    difference.insert(cdep);
                }
            }
        }
    }

    // }
    return difference;
}

Instruction *DepWalker::getBranchInst(llvm::BasicBlock *bb) {
    llvm::Instruction *termInst = bb->getTerminator();
    if (llvm::BranchInst *branchInst =
            llvm::dyn_cast<llvm::BranchInst>(termInst)) {
        if (branchInst->isConditional()) {
            // errs()<<"has con\n";
            return branchInst;
        } else {
            // errs()<<"no con\n";
        }
    }
    return nullptr;
}