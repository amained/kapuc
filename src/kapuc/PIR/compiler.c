#include "compiler.h"
#include "lib/log.h"

#include "llvm-c/Core.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Types.h"
#include <assert.h>
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
    LLVMValueRef main_call = LLVMBuildCall2(builder2, t, main, NULL, 0, "main");
    LLVMValueRef arg[] = { main_call };
    LLVMBuildCall2(builder2, exit_type, exit_func, arg, 1, "");
    LLVMBuildUnreachable(builder2);
}

#define TYSWITCH(a,b,c) case a: {\
  c;\
  break;\
}

static LLVMValueRef resolve_static_val(val* v) {
  log_debug("resolving static value of type %d", v->t);
  assert(v->t.is_default_type);
  switch(v->t.default_type) {
    #define INTTYVAL(a,b) TYSWITCH(a,b,return LLVMConstInt(b, v->int__val, false))
    INTTYVAL(0, LLVMInt8Type())
    INTTYVAL(1, LLVMInt16Type())
    INTTYVAL(2, LLVMInt32Type())
    INTTYVAL(3, LLVMInt64Type())
    default: return NULL;
    #undef INTTYVAL
  }
}

// for func_val <-> LLVMValueRef
// we can use vec for this since func_var is supposed to be linear anyways
typedef struct {bool isAlloca_ed; LLVMValueRef v;} FuncVarReg;
#define T FuncVarReg
void FuncVarReg_free(T*) {
  // we do nothing since we can just dispose entire module for that
}
T
FuncVarReg_copy(T* V) {
  FuncVarReg* V2 = malloc(sizeof(FuncVarReg)); // I mean it's just pointer?? also this function shouldn't be called anyways?
  memcpy(V2, V, sizeof(FuncVarReg));
  return *V2;
}
#include "lib/ctl/vec.h"
#undef T

static inline LLVMValueRef resolve_val(expr* e, vec_FuncVarReg* v, LLVMBuilderRef b) {
  switch(e->t) {
    case Val: return resolve_static_val(&e->v);
    case Func_val: {
      FuncVarReg* smol_v = vec_FuncVarReg_at(v, e->func_val);
      if (smol_v->isAlloca_ed) 
        return LLVMBuildLoad2(b, LLVMInt8Type(), smol_v->v, ""); // Internal Compiler Crash if NULL, FIXME: rewrite this shit
      return smol_v->v;
    }
    default: return NULL;
  }
}

LLVMModuleRef generate_LLVM_IR(struct PIR* p, char* module_name) {
    LLVMModuleRef module = LLVMModuleCreateWithName(module_name);
    foreach(vec_MAIN_BLOCK, &p->main_blocks, iter) {
        switch(iter.ref->type) {
            case func: {
                // check typing
                assert(iter.ref->func.t.is_default_type);
                LLVMTypeRef ret_type = NULL;
                LLVMBuilderRef builder = LLVMCreateBuilder();
                switch(iter.ref->func.t.default_type) {
#define FUNCTY(a,b) TYSWITCH(a,b,ret_type = LLVMFunctionType(b, NULL, 0, 0));
                    FUNCTY(0,LLVMInt8Type());
                    FUNCTY(1,LLVMInt16Type());
                    FUNCTY(2,LLVMInt32Type());
                    FUNCTY(3,LLVMInt64Type());
#undef FUNCTY
                }
                LLVMValueRef f = LLVMAddFunction(module, iter.ref->func.name, ret_type);
                char val[15]; // TODO: figure out why 15
                size_t current_pos = 0;
                vec_FuncVarReg v = vec_FuncVarReg_init();
                foreach(vec_BLOCK, &iter.ref->func.bs, iter2) {
                  sprintf(val, "$%zu", current_pos); // TODO: add some debug info here on debug build? Incase the compiler f-up we could check the IR
                  LLVMBasicBlockRef block = LLVMAppendBasicBlock(f, val);
                  LLVMPositionBuilderAtEnd(builder, block);
                  foreach(vec_stmt, iter2.ref, iter3) {
                    log_debug("checking");
                    switch(iter3.ref->t) {
                      case assignment: {
                        FuncVarReg* f = malloc(sizeof(FuncVarReg));
                        log_debug("found assignment to _%d as %d", iter3.ref->assignment.id, iter3.ref->assignment.e.t);
                        switch(iter3.ref->assignment.e.t) {
                          case Val: {
                            log_debug("Val!");
                            #define ASSIGNTY(a,b) TYSWITCH(a,b,lhs = LLVMBuildAlloca(builder, b, ""));
                            assert(iter3.ref->assignment.e.v.t.is_default_type);
                            LLVMValueRef lhs;
                            switch (iter3.ref->assignment.e.v.t.default_type) {
                              ASSIGNTY(0, LLVMInt8Type())
                              ASSIGNTY(1, LLVMInt16Type())
                              ASSIGNTY(2, LLVMInt32Type())
                              ASSIGNTY(3, LLVMInt64Type())
                            }
                            LLVMBuildStore(builder, resolve_static_val(&iter3.ref->assignment.e.v), lhs);
                            f->isAlloca_ed = true;
                            f->v = lhs;
                            break;
                          }
                          default: {log_debug("what %d", iter3.ref->assignment.e.t);}
                        }
                        vec_FuncVarReg_push_back(&v, *f);
                        continue;
                      }
                      case ret: {
                        log_debug("found ret!");
                        LLVMBuildRet(builder, resolve_val(&iter3.ref->ret_val, &v, builder));
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