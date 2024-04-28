//===- TimingInstrumenter.cpp
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
#include <llvm/IR/DebugInfo.h>

#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <vector>

// #include "MemoryModel/PointerAnalysis.h"
// #include "MemoryModel/PointerAnalysisImpl.h"
// #include "SVF-FE/LLVMUtil.h"
// #include "Util/Options.h"
// #include "WPA/WPAPass.h"
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
#include "llvm/ADT/SetVector.h"
#include <nlohmann/json.hpp>

#include "dependencyWalker.h"

using json = nlohmann::json;

using namespace std;
// using namespace SVF;

namespace {

using namespace llvm;

struct TimingInstrumenter : public ModulePass {
    static char ID;


    int _timing_vertex_index = 0;
    std::vector<llvm::Function *> _annotated_functions; // TO REMOVE
    std::map<std::string, std::string> _name_to_ir_name_map;
    std::string _graph_file_path = "/home/cspl/Shore-user/TDG-template.json";
    std::string _id_index_file_path = "/tmp/id_index.txt";

    std::vector<std::string> _input_task_names;
    // std::vector<llvm::Function *> _input_callback_functions;
    std::map<llvm::Function*, int> _input_function_to_id_map;
    std::map<int, std::vector<llvm::Function *>> _id_input_function_map;
    std::map<std::string, int> _input_function_name_to_id_map;

    // Relations
    std::map<std::string, std::vector<std::string>> _function_to_input_dependency_map;
    std::map<int, std::vector<int>> _vertex_dependency_map;

    // SVF-based data-flow analysis
    DepWalker* dependency_walker;


    TimingInstrumenter() : ModulePass(ID) {
        // initializeTimingInstrumenterPass(*PassRegistry::getPassRegistry());
    }

    bool runOnModule(Module &M) override {
        bool is_transformed = false;

        errs() << "[Shore-Pass] Hello, in the Timiming instrumenter LLVM pass\n";

        initialization(M);

        // Perform the points-to based data-flow analysis
        // dataFlowAnalysisOnPAG(M);

        // Insert the update of necessary vertices

        is_transformed |= instrumentTimingDefUpdateByGraph(M);

        is_transformed |= instrumentTimingChecks(M);

        dumpVertexDependencyMap();

        // Insert the initialization function at the end, we need the number of
        // vertices
        is_transformed = instrumentGraphInitialization(M);

        writeTimingVertexIndex();

        return is_transformed;
    
    }

    void initialization(Module &M);

    void dataFlowAnalysisOnPAG(Module &M);

    bool instrumentTimingDefUpdateByGraph(Module &M);

    bool instrumentTimingChecks(Module &M);

    bool instrumentGraphInitialization(Module &M);

    void dumpVertexDependencyMap();

    int writeTimingVertexIndex();

    Constant *getFunctionPointer(Module &M, const std::string &func_name);
};


void TimingInstrumenter::dataFlowAnalysisOnPAG(Module &M) {
    // Perform the points-to based data-flow analysis

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

    /// Sparse value-flow graph (SVFG)
    SVFGBuilder svfBuilder(true);
    SVFG* svfg = svfBuilder.buildFullSVFG(ander);

    // The init function is:
    // DepWalker(llvm::Module& m, SVFModule* svfModule, SVFIR* pag,
    //           SymbolTableInfo::ValueToIDMapTy& valSymMap, SVFG* svfg)

    dependency_walker = new DepWalker(M, svfModule, pag, SymbolTableInfo::SymbolInfo()->valSyms(), svfg);

    // Walk through
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

                        if (called_function && (called_function->getName().contains("freshness") ||
                                                called_function->getName().contains("consistency") ||
                                                called_function->getName().contains("stability"))) {
                            is_annotated = 1;
                        }
                    
                        if (is_annotated) {
                            errs() << "Annotated Instruction: " << I << "\n";
                            /////////////////////////////////
                            // Generate dataflow graph
                            /////////////////////////////////
                            SVF::PAG* pag = vfg->getPAG();
                            if (pag->hasValueNode(&I)) {
                                errs() << "Found Instruction: " << I << " in the SVF graph \n";
                                
                                // Get the parameter of the instruction
                                llvm::Value *param = I.getOperand(0);

                                errs() << "Hello, Parameter: " << *param << "\n";

                                SVF::NodeID nodeId = pag->getValueNode(&I);
                                SVF::VFGNode* node = vfg->getVFGNode(nodeId);
                                Set<SVF::VFGNode *> defs = dependency_walker->getVFGNodeUpperDefs(node, false);
                                
                                // for (auto def : defs) {
                                //     errs() << "Def: " << def->toString() << "\n";
                                // }


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



}


void TimingInstrumenter::initialization(Module &M) {
    /// Get the current index of the timing vertex from file
    std::ifstream index_file(_id_index_file_path);
    if (!index_file.is_open()) {
        std::cerr << "Failed to open file: " << _id_index_file_path
                  << std::endl;
        errs() << "Assgining index from 0\n";
    } else {
        index_file >> _timing_vertex_index;
        index_file.close();
        errs() << "The current index is: " << _timing_vertex_index << "\n";
    }

    /////// load the graph from the file
    std::ifstream grah_file(_graph_file_path);
    if (!grah_file.is_open()) {
        std::cerr << "Failed to open file: " << _graph_file_path << std::endl;
        return;
    }

    nlohmann::json json;
    grah_file >> json;
    grah_file.close();

    if (json.contains("dependency")) {
        for (auto &item : json["dependency"].items()) {
            //             std::cout << "Key: " << key << std::endl;
            std::string key = item.key();
            auto values = item.value();
            std::string function_name = key;
            std::vector<std::string> dependencies;

            if (values.contains("deps")) {
                //               std::cout << "Dependencies: ";
                // Iterate through the dependencies array
                for (auto &dep : values["deps"]) {
                    //                       std::cout << dep << " ";
                    dependencies.push_back(dep);
                }
                _function_to_input_dependency_map[function_name] = dependencies;
                //                   std::cout << std::endl;
            }
        }
    }

    /////// Get all the input callback functions
    auto input_tasks = json["input-tasks"];
    //       std::cout << "Input Tasks:" << std::endl;
    for (const auto &task : input_tasks) {
        _input_task_names.push_back(task.get<std::string>());
        //           std::cout << "- " << task.get<std::string>() << std::endl;
    }
    return;
}

bool TimingInstrumenter::instrumentGraphInitialization(Module &M) {

    LLVMContext &Context = M.getContext();

    // Function *main_function;
    for (Function &F : M) {
        if (F.isDeclaration()) {
            continue;
        }

        if (F.getName().contains("main")) {
            errs() << "Main function found. The name in IR is : " << F.getName()
                   << " \n";

            llvm::IRBuilder<> Builder(&F.getEntryBlock().front());

            // Instrument to set the graph size
            FunctionCallee SetGraphSizeFunc =
                F.getParent()->getOrInsertFunction(
                    "setGraphSize",
                    FunctionType::get(Type::getVoidTy(Context),
                                      {Type::getInt32Ty(Context)}, false));

            Value *graphSizeValue = ConstantInt::get(Type::getInt32Ty(Context),
                                                     _timing_vertex_index + 1);
            Builder.CreateCall(SetGraphSizeFunc, {graphSizeValue});

            // Instrument to initialize the timing graph
            FunctionCallee init_timing_func = M.getOrInsertFunction(
                "initTimingGraph",
                FunctionType::get(Type::getVoidTy(Context), false));

            Builder.CreateCall(init_timing_func);


            errs() << "Initlization related function are instrumented\n";
            // Instrument to get the callback function ptrs

            for (std::string &name : _input_task_names) {
                std::vector<Type *> FuncArgs;
                FuncArgs.push_back(Type::getInt32Ty(Context));  // int id
                FuncArgs.push_back(
                    Type::getInt8PtrTy(Context));  // void* func (as int8*)

                FunctionType *FuncType = FunctionType::get(
                    Type::getVoidTy(Context), FuncArgs, false);

                FunctionCallee addFunction = M.getOrInsertFunction(
                    "registerROSCallbackFunctionPtr", FuncType);

                int id = _input_function_name_to_id_map[name];

                // std::map<llvm::Function*, int> _input_function_to_id_map;
                Constant *funcPtr;
                for (auto &pair : _input_function_to_id_map) {
                    if (pair.second == id) {
                        funcPtr =
                            getFunctionPointer(M, pair.first->getName().str());
                    }
                }

                // Constant *funcPtr = getFunctionPointer(M, name);
                if (!funcPtr) {
                    errs() << "Failed to get function pointer for " << name
                           << "\n";
                    return false;
                }

                // Value *id_value = ConstantInt::get(Type::getInt32Ty(Context),
                // _input_function_name_to_id_map[name]);

                Builder.CreateCall(addFunction,
                                   {Builder.getInt32(id), funcPtr});

                errs() << "Inserted callback adding function for " << name
                       << "\n";
            }

            return true;
        }
    }

    return false;
}

bool TimingInstrumenter::instrumentTimingDefUpdateByGraph(Module &M) {
    bool ret_transformed = false;

    /////// Instrument DEF timing update in input callback functions
    // Prepare the function to be inserted
    LLVMContext &Ctx = M.getContext();

    // For getting the timestamp from ROS message
    // FunctionCallee updateDefTimingFunc =
    //     M.getOrInsertFunction("updateDefTimingByID",
    //                           Type::getInt32Ty(Ctx),   // Return type
    //                           Type::getInt32Ty(Ctx));  // Parameter type

    FunctionCallee updateInputDefTimingFunc = M.getOrInsertFunction(
        "updateInputDefTimeByID",
        FunctionType::get(Type::getInt32Ty(Ctx),
                          {Type::getInt32Ty(Ctx), Type::getInt8PtrTy(Ctx)},
                          false));

    // For initializing the function pointers
    // Assume all functions have the same type as the first one
    StructType *entryType = StructType::get(
        Ctx,
        {Type::getInt32Ty(Ctx), PointerType::get(Type::getVoidTy(Ctx), 0)});
    std::vector<Constant *> idFuncEntries;

    // Start iterating over the functions
    for (Function &F : M) {
        if (F.isDeclaration()) continue;

        for (std::string &name : _input_task_names) {
            if (F.getName().contains(name)) {
                errs() << "Input callback task found. The name in IR is : "
                       << F.getName() << " | the current vertex ID is "
                       << _timing_vertex_index << " \n";

                llvm::IRBuilder<> Builder(Ctx);

                /***************************************************************************************
                 * For Parsing the timestamp from ROS message
                 **************************************************************************************
                 */

                for (auto &BB : F) {
                    Builder.SetInsertPoint(&BB, BB.getFirstInsertionPt());

                    Value *id_value = ConstantInt::get(Type::getInt32Ty(Ctx),
                                                       _timing_vertex_index);

                    Value *imuMsgArg = F.getArg(1);

                    //     Value *imuMsgArg = F.arg_begin();
                    Value *castedPtr = Builder.CreateBitCast(
                        imuMsgArg, PointerType::getUnqual(
                                       Type::getInt8Ty(M.getContext())));

                    Builder.CreateCall(
                        updateInputDefTimingFunc,
                        {id_value, castedPtr});  // Call FunctionB with the
                                                 // address of the first
                                                 // parameter

                    errs() << "Inserted updateDefTimingByID at the "
                              "beginning of the input callback function\n";

                    break;  // Only insert at the first block
                }

                /***************************************************************************************
                 * For Initializing the callback function pointers, which is
                 *used to distinguish the ros workers TODO
                 **************************************************************************************
                 */

                Constant *id_constant = ConstantInt::get(Type::getInt32Ty(Ctx),
                                                         _timing_vertex_index);
                Constant *func_ptr = ConstantExpr::getBitCast(
                    &F, PointerType::get(Type::getVoidTy(Ctx), 0));
                idFuncEntries.push_back(
                    ConstantStruct::get(entryType, {id_constant, func_ptr}));

                /*
                 *****************************************************************
                 * End - Initializing the callback function pointers
                 *****************************************************************
                 */

                // Builder.CreateCall(updateInputDefTimingFunc, {id_value,
                // nullptr});

                _input_function_to_id_map.insert({&F, _timing_vertex_index});

                _input_function_name_to_id_map.insert(
                    {name, _timing_vertex_index});

                _timing_vertex_index++;
                ret_transformed = true;
            }
        }
    }

    return ret_transformed;
}

bool TimingInstrumenter::instrumentTimingChecks(Module &M) {
    bool ret_transformed = false;

    int timing_property = 0;
    int parameters = 0;
    int violation_policy = -1;  // FIXME

    for (Function &F : M) {
        for (BasicBlock &BB : F) {
            for (Instruction &I : BB) {
                int is_annotated = 0;

                if (isa<CallInst>(I) || isa<InvokeInst>(I)) {
                    // Find call and invoke instructions
                    Function *called_function;

                    if (auto *callInst = dyn_cast<CallInst>(&I)) {
                        // errs() << "Call instruction is " <<
                        // *callInst <<
                        // "\n";
                        called_function = callInst->getCalledFunction();
                    } else if (auto *invokeInst = dyn_cast<InvokeInst>(&I)) {
                        called_function = invokeInst->getCalledFunction();
                    }

                    if (called_function &&
                        called_function->getName().contains("freshness")) {
                        errs() << "Call to 'FRESHNESS' found in "
                                  "function '"
                               << F.getName() << "'\n";
                        timing_property = 1;
                        is_annotated = 1;
                        parameters = 1;
                    } else if (called_function &&
                               called_function->getName().contains(
                                   "consistency")) {
                        errs() << "Call to 'CONSISTENCY' found in "
                                  "function '"
                               << F.getName() << "'\n";
                        timing_property = 2;
                        is_annotated = 1;
                        parameters = 2;
                        if (called_function->getName().contains(
                                "consistency3")) {
                            parameters = 3;
                        }

                    } else if (called_function &&
                               called_function->getName().contains(
                                   "stability")) {
                        errs() << "Call to 'STABILITY' found in "
                                  "function '"
                               << F.getName() << "'\n";
                        timing_property = 3;
                        is_annotated = 1;
                        parameters = 1;
                    }

                    // This isDeclaration() is not necessary in this
                    // context, TO REMOVE
                    // if (is_annotated &&
                    // !called_function->isDeclaration()) {
                    if (is_annotated) {
                        /*
                         // * Get the original variable name from the debug info
                         // *
                                     if (auto *DVI = dyn_cast<DbgValueInst>(&I))
                         { if (DVI->getValue()) { auto *DIVar =
                         DVI->getVariable(); if (DIVar) { StringRef VarName =
                         DIVar->getName();
                                                 // Now VarName holds the
                         original
                                                 // variable name You can use
                         VarName as
                                                 // needed here
                                                 errs() << "Variable name is : "
                                                        << VarName.str() <<
                         "\n";
                                             }
                                         }
                                     } else {
                                         errs() << "Cannot convert, Instruction
                         is : " << I << "\n";
                                     }
                         */
                        //
                        // Instrument the timing checking functions
                        //

                        /// Add new code
                        LLVMContext &Ctx = M.getContext();
                        llvm::IRBuilder<> Builder(Ctx);
                        Builder.SetInsertPoint(&I);  // Set the insertion point
                        // FIXME: This is a hack to move the
                        // insertion
                        // Builder.SetInsertPoint(
                        //     &BB,
                        //     ++Builder.GetInsertPoint());  // Move to
                        // next
                        // instruction

                        FunctionCallee checkTimingFunc = M.getOrInsertFunction(
                            "checkTimingCorrectnessByID",
                            FunctionType::get(IntegerType::getInt32Ty(Ctx),
                                              {IntegerType::getInt32Ty(Ctx),
                                               IntegerType::getInt32Ty(Ctx),
                                               Type::getDoubleTy(Ctx),
                                               IntegerType::getInt32Ty(Ctx)},
                                              false)  // violation_policy
                        );

                        // Arguments to the function call
                        Value *id = ConstantInt::get(Type::getInt32Ty(Ctx),
                                                     _timing_vertex_index);
                        Value *property = ConstantInt::get(
                            Type::getInt32Ty(Ctx), timing_property);

                        Value *threshold;
                        Value *violation_policy;
                        if (parameters == 1) {
                            // This is freshness and stability
                            threshold = I.getOperand(1);
                            violation_policy = I.getOperand(2);
                        } else if (parameters == 2) {
                            // This is consistency
                            threshold = I.getOperand(2);
                            violation_policy = I.getOperand(3);
                        } else if (parameters == 3) {
                            // This is consistency3
                            threshold = I.getOperand(3);
                            violation_policy = I.getOperand(4);
                        }

                        // Insert the call
                        Value *ret_value = Builder.CreateCall(
                            checkTimingFunc,
                            {id, property, threshold, violation_policy});

                        // Insert code for handling abort
                        llvm::GlobalVariable *gloabl_should_abort_var =
                            M.getNamedGlobal("shouldAbort");
                        Builder.CreateStore(ret_value, gloabl_should_abort_var);

                        // End - ABORT policy

                        errs() << "Instrumented timing check function\n";
                        // End of Instrumentation

                        /*
                         * ****************************************************************
                         * Store the timing vertex id dependecy relations
                         * ****************************************************************
                         */

                        // Get the current function
                        Function *current_function = I.getFunction();
                        // Get the function name
                        StringRef function_name = current_function->getName();

                        // find the original name of the function
                        for (auto &pair : _function_to_input_dependency_map) {
                            if (function_name.contains(pair.first)) {
                                // Get the vertex id
                                for (std::string dep_function_name :
                                     pair.second) {
                                    errs()
                                        << "The dependency function name is : "
                                        << dep_function_name << "\n";

                                    int dep_vertex_id =
                                        _input_function_name_to_id_map
                                            [dep_function_name];

                                    errs() << "The depenedency vertex id is: "
                                           << dep_vertex_id << "\n";

                                    // Store the dependency
                                    _vertex_dependency_map[_timing_vertex_index]
                                        .push_back(dep_vertex_id);
                                }
                            }
                        }

                        _timing_vertex_index++;
                        ret_transformed = true;
                    }
                }
            }
        }
    }
    // Store _vertex_dependency_map
    if (!_vertex_dependency_map.empty()) {
        dumpVertexDependencyMap();
    }

    return ret_transformed;
}

void TimingInstrumenter::dumpVertexDependencyMap() {
    std::ofstream output_file("/tmp/vertex_dependency_map.txt");
    if (output_file.is_open()) {
        for (const auto &entry : _vertex_dependency_map) {
            output_file << entry.first;
            output_file << " -> ";
            for (int dep_vertex_id : entry.second) {
                output_file << dep_vertex_id << " ";
            }
            output_file << "\n\n";
            output_file.close();
        }
        errs() << "Stored _vertex_dependency_map in "
                  "vertex_dependency_map.txt\n";
    } else {
        errs() << "Failed to open file for writing\n";
    }
}

int TimingInstrumenter::writeTimingVertexIndex() {
    std::ofstream file(_id_index_file_path);
    if (!file.is_open()) {
        std::cerr << "Failed to open file: " << _id_index_file_path
                  << std::endl;
        return -1;
    }

    file << _timing_vertex_index;
    file.close();
    return 0;
}

Constant* TimingInstrumenter::getFunctionPointer(Module &M, const std::string &func_name) {
    Function *func = M.getFunction(func_name);
    if (!func) {
        errs() << "Function " << func_name << " not found in module.\n";
        return nullptr;
    }
    return ConstantExpr::getBitCast(func, Type::getInt8PtrTy(M.getContext()));
}

}  // namespace

char TimingInstrumenter::ID = 0;

static llvm::RegisterPass<TimingInstrumenter> X("TimingInstrumenter",
                                                "TimingInstrumenter Pass",
                                                false /* Only looks at CFG */,
                                                false /* Analysis Pass */);

static llvm::RegisterStandardPasses Y(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    [](const llvm::PassManagerBuilder &Builder,
       llvm::legacy::PassManagerBase &PM) {
        PM.add(new TimingInstrumenter());
    });
