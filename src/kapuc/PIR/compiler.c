#include "compiler.h"
#include "lib/log.h"

#include "llvm-c/Core.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Types.h"
#include <stdio.h>
#include <string.h>

void add_default__start_func(LLVMModuleRef module, LLVMValueRef main, LLVMTypeRef t) {
    LLVMTypeRef exit_arg_types[] = { LLVMInt32Type() };
    LLVMTypeRef exit_type =
      LLVMFunctionType(LLVMVoidType(), exit_arg_types, 1, 0);
    LLVMValueRef exit_func = LLVMAddFunction(module, "exit", exit_type);

    LLVMBuilderRef builder2 = LLVMCreateBuilder();
    LLVMValueRef _start_func = LLVMAddFunction(
      module, "_start", LLVMFunctionType(LLVMVoidType(), NULL, 0, 0));
    LLVMBasicBlockRef _start_block =
      LLVMAppendBasicBlock(_start_func, "_start_real_entry");
    LLVMPositionBuilderAtEnd(builder2, _start_block);
    LLVMBuildCall2(builder2, t, main, NULL, 0, "main");
    LLVMValueRef arg[] = { LLVMConstInt(LLVMInt32Type(), 0, 0) };
    LLVMBuildCall2(builder2, exit_type, exit_func, arg, 1, "");
    LLVMBuildUnreachable(builder2);
}

#define FUNCTY(a,b) case a: {\
    ret_type = LLVMFunctionType(b, NULL, 0, 0);\
    break;\
}

LLVMModuleRef generate_LLVM_IR(struct PIR* p, char* module_name) {
    LLVMModuleRef module = LLVMModuleCreateWithName(module_name);
    foreach(vec_MAIN_BLOCK, &p->main_blocks, iter) {
        switch(iter.ref->type) {
            case func: {
                // check typing
                if (!iter.ref->func.t.is_default_type) exit(1);
                LLVMTypeRef ret_type = NULL;
                LLVMBuilderRef builder = LLVMCreateBuilder();
                switch(iter.ref->func.t.default_type) {
                    FUNCTY(0,LLVMInt8Type());
                    FUNCTY(1,LLVMInt16Type());
                    FUNCTY(2,LLVMInt16Type());
                }
                LLVMValueRef f = LLVMAddFunction(module, iter.ref->func.name, ret_type);
                char val[15]; // TODO: figure out why 15
                size_t current_pos = 0;
                foreach(vec_BLOCK, &iter.ref->func.bs, iter2) {
                  sprintf(val, "$%zu", current_pos); // TODO: add some debug info here on debug build? Incase the compiler f-up we could check the IR
                  LLVMBasicBlockRef block = LLVMAppendBasicBlock(f, val);
                  LLVMPositionBuilderAtEnd(builder, block);
                  foreach(vec_stmt, iter2.ref, iter3) {
                    log_debug("checking");
                    switch(iter3.ref->t) {
                      case assignment: {
                        log_debug("found assignment to _%d as %d", iter3.ref->assignment.id, iter3.ref->assignment.e.t);
                        LLVMValueRef lhs = LLVMBuildAlloca(builder, LLVMInt8Type(), "ee");
                        switch(iter3.ref->assignment.e.t) {
                          case Val: {
                            log_debug("Val!");
                            LLVMBuildStore(builder, LLVMConstInt(LLVMInt8Type(), 0, false), lhs);
                            break;
                          }
                          default: {log_debug("what %d", iter3.ref->assignment.e.t);}
                        }
                        continue;
                      }
                      case ret: {
                        log_debug("found ret!");
                        // TODO: resolve value with a universal function
                        // something like LLVMValueRef resolve_value(...)
                        switch(iter3.ref->ret_val.t) {
                          case Val: {
                            log_debug("Val!");
                            LLVMBuildRet(builder, LLVMConstInt(LLVMInt8Type(), 2, false));
                            break;
                          }
                          default: {log_debug("what %d", iter3.ref->ret_val.t);}
                        }
                        continue;
                      }
                      default: {
                        log_debug("found weird shit at pos %zu", current_pos);
                        break;
                      }
                    }
                  }
                  current_pos++;
                }
                if (strcmp(iter.ref->func.name, "main") == 0) {
                    printf("found main!\n");
                    add_default__start_func(module, f, ret_type); // the PIR should have only 1 main anyways
                }
                continue;
            }
        }
    }
    log_debug("successfully generate LLVM Module");
    return module;
}

void compile_module(LLVMModuleRef module, char* name) {
    char* triple = LLVMGetDefaultTargetTriple();
    LLVMTargetRef t = NULL;
    char* error = NULL;
    LLVMBool l = LLVMGetTargetFromTriple(triple, &t, &error);
    if (t == NULL || l != 0) {
        log_error("failed to get target: %s", error);
        LLVMDisposeMessage(error);
        LLVMDisposeMessage(triple);
        exit(1);
    }
    LLVMTargetMachineRef machine = LLVMCreateTargetMachine(
      t,
      triple,
      "generic",
      "",
      LLVMCodeGenLevelAggressive,
      LLVMRelocDynamicNoPic,
      LLVMCodeModelMedium);
    // TODO: link here
    LLVMTargetMachineEmitToFile(
      machine, module, name, LLVMObjectFile, NULL);
    LLVMDisposeMessage(triple);
    LLVMDisposeTargetMachine(machine);
}