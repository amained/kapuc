#include "compiler.h"

#include "lib/log.h"

#include "llvm-c/Core.h"
#include "llvm-c/TargetMachine.h"
#include "llvm-c/Types.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>

static LLVMBuilderRef
add_default__start_func(LLVMModuleRef module, LLVMValueRef main, LLVMTypeRef t)
{
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
    return builder2;
}

#define TYSWITCH(a, b, c)                                                      \
    case a: {                                                                  \
        c;                                                                     \
        break;                                                                 \
    }

#define ALL_TYPE(M)                                                            \
    M(0, LLVMInt8Type())                                                       \
    M(1, LLVMInt16Type())                                                      \
    M(2, LLVMInt32Type()) M(3, LLVMInt64Type()) M(4, LLVMInt1Type())

static inline LLVMValueRef
_resolve_static_val(val* v)
{
    log_debug("resolving static value of type %d", v->t);
    assert(v->t.is_default_type);
    switch (v->t.default_type) {
#define INTTYVAL(a, b)                                                         \
    TYSWITCH(a, b, return LLVMConstInt(b, v->int__val, false))
        ALL_TYPE(INTTYVAL)
    default:
        return NULL;
#undef INTTYVAL
    }
}

static LLVMValueRef
resolve_static_val(val* v, LLVMBuilderRef b)
{
    if (v->t.is_ptr)
        return LLVMBuildIntToPtr(b, _resolve_static_val(v), LLVMInt8Type(), "");
    return _resolve_static_val(v);
}

// for func_val <-> LLVMValueRef
// we can use vec for this since func_var is supposed to be linear anyways
typedef struct
{
    bool isAlloca_ed;
    LLVMValueRef v;
} FuncVarReg;
#define T FuncVarReg
void
FuncVarReg_free(T*)
{
    // we do nothing since we can just dispose entire module for that
}
T
FuncVarReg_copy(T* V)
{
    FuncVarReg* V2 =
      malloc(sizeof(FuncVarReg)); // I mean it's just pointer?? also this
                                  // function shouldn't be called anyways?
    memcpy(V2, V, sizeof(FuncVarReg));
    return *V2;
}
#include "lib/ctl/vec.h"
#undef T

// for func <-> LLVMValueRef
typedef struct
{
    bool is_external;
    LLVMValueRef v;
} Func;
#define T Func
void
Func_free(T*)
{
    // we do nothing since we can just dispose entire module for that
}
T
Func_copy(T* V)
{
    Func* V2 = malloc(sizeof(Func)); // I mean it's just pointer?? also this
                                     // function shouldn't be called anyways?
    memcpy(V2, V, sizeof(Func));
    return *V2;
}
#include "lib/ctl/vec.h"
#undef T

static inline LLVMValueRef
resolve_val(expr* e, vec_FuncVarReg* v, LLVMBuilderRef b)
{
    switch (e->t) {
    case Val:
        return resolve_static_val(&e->v, b);
    case Func_val: {
        FuncVarReg* smol_v = vec_FuncVarReg_at(v, e->func_val);
        if (smol_v->isAlloca_ed)
            return LLVMBuildLoad2(b, LLVMInt8Type(), smol_v->v, "");
        return smol_v->v;
    }
    default:
        return NULL;
    }
}
#define T LLVMValueRef
void
LLVMValueRef_free(T*)
{
    // it's a ref anyways?
}
T
LLVMValueRef_copy(T* v)
{
    return *v;
}
#include "lib/ctl/vec.h"
#undef T

#define T LLVMBuilderRef
void
LLVMBuilderRef_free(T* v)
{
    LLVMDisposeBuilder(*v);
}
T
LLVMBuilderRef_copy(T* v)
{
    return *v;
}
#include "lib/ctl/vec.h"
#undef T

LLVMModuleRef
generate_LLVM_IR(struct PIR* p, char* module_name)
{
    LLVMModuleRef module = LLVMModuleCreateWithName(module_name);
    vec_Func fs = vec_Func_init();
    vec_LLVMBuilderRef b_ref = vec_LLVMBuilderRef_init();
    foreach (vec_MAIN_BLOCK, &p->main_blocks, iter) {
        switch (iter.ref->type) {
        case func: {
            // check typing
            assert(iter.ref->func.t.is_default_type);
            LLVMTypeRef ret_type = NULL;
            LLVMBuilderRef builder = LLVMCreateBuilder();
            switch (iter.ref->func.t.default_type) {
#define FUNCTY(a, b) TYSWITCH(a, b, ret_type = b);
                ALL_TYPE(FUNCTY)
#undef FUNCTY
            }
            ret_type =
              LLVMFunctionType(ret_type, NULL, 0, iter.ref->func.is_variadic);
            LLVMValueRef f =
              LLVMAddFunction(module, iter.ref->func.name, ret_type);
            char* type_string = LLVMPrintTypeToString(ret_type);
            log_debug("type: %s", type_string);
            LLVMDisposeMessage(type_string);
            Func* fi = malloc(sizeof(Func));
            fi->is_external = iter.ref->func.is_external;
            fi->v = f;
            vec_Func_push_back(&fs, *fi);
            if (!iter.ref->func.is_external) {
                char val[15]; // TODO: figure out why 15
                size_t current_pos = 0;
                vec_FuncVarReg v = vec_FuncVarReg_init();
                foreach (vec_BLOCK, &iter.ref->func.bs, iter2) {
                    sprintf(val,
                            "$%zu",
                            current_pos); // TODO: add some debug info here on
                                          // debug build? Incase the compiler
                                          // f-up we could check the IR
                    LLVMBasicBlockRef block = LLVMAppendBasicBlock(f, val);
                    LLVMPositionBuilderAtEnd(builder, block);
                    foreach (vec_stmt, iter2.ref, iter3) {
                        log_debug("checking");
                        switch (iter3.ref->t) {
                        case assignment: {
                            FuncVarReg* f = malloc(sizeof(FuncVarReg));
                            log_debug("found assignment to _%d as %d",
                                      iter3.ref->assignment.id,
                                      iter3.ref->assignment.e.t);
                            switch (iter3.ref->assignment.e.t) {
                            case Func_val:
                            case Val: {
                                log_debug("Val!");
#define ASSIGNTY(a, b) TYSWITCH(a, b, lhs = LLVMBuildAlloca(builder, b, ""));
                                if (iter3.ref->assignment.e.t == Val)
                                    assert(iter3.ref->assignment.e.v.t
                                             .is_default_type);
                                LLVMValueRef lhs;
                                switch (
                                  iter3.ref->assignment.e.v.t.default_type) {
                                    ALL_TYPE(ASSIGNTY)
                                }
                                LLVMBuildStore(
                                  builder,
                                  resolve_val(
                                    &iter3.ref->assignment.e, &v, builder),
                                  lhs);
                                f->isAlloca_ed = true;
                                f->v = lhs;
                                break;
                            }
                            default: {
                                // Add and stuff
                                log_debug("what %d", iter3.ref->assignment.e.t);
                                LLVMValueRef val;
#define GENLLVMBUILDE(E, E2)                                                   \
    case E: {                                                                  \
        val = LLVMBuild##E2(                                                   \
          builder,                                                             \
          resolve_val(iter3.ref->assignment.e.b.lhs, &v, builder),             \
          resolve_val(iter3.ref->assignment.e.b.rhs, &v, builder),             \
          "");                                                                 \
        break;                                                                 \
    }
                                switch (iter3.ref->assignment.e.t) {
                                    GENLLVMBUILDE(Add, Add)
                                    GENLLVMBUILDE(Mul, Mul)
                                    GENLLVMBUILDE(Div, SDiv)
                                    GENLLVMBUILDE(Del, Sub)
                                default:
                                    assert(false);
                                }
                                f->isAlloca_ed = false;
                                f->v = val;
                                break;
                            }
                            }
                            vec_FuncVarReg_push_back(&v, *f);
                            continue;
                        }
                        case ret: {
                            log_debug("found ret!");
                            LLVMBuildRet(
                              builder,
                              resolve_val(&iter3.ref->ret_val, &v, builder));
                            continue;
                        }
                        case call: {
                            log_debug("found call!");
                            Func* fi =
                              vec_Func_at(&fs, iter3.ref->call_ca.call_ids);
                            if (fi == NULL) {
                                log_error("Failed to resolve function, is it "
                                          "created yet?");
                                log_debug("Continuing from error");
                                continue;
                            }
                            vec_LLVMValueRef wtf = vec_LLVMValueRef_init();
                            foreach (
                              vec_expr, &iter3.ref->call_ca.value, iter4) {
                                vec_LLVMValueRef_push_back(
                                  &wtf, resolve_val(iter4.ref, &v, builder));
                            }
                            LLVMBuildCall2(builder,
                                           LLVMGlobalGetValueType(fi->v),
                                           fi->v,
                                           wtf.value,
                                           wtf.size,
                                           ""); // this is always global anyways
                            continue;
                        }
                        default: {
                            log_debug("found weird shit at pos %zu",
                                      current_pos);
                            break;
                        }
                        }
                    }
                    current_pos++;
                }
                if (strcmp(iter.ref->func.name, "main") == 0) {
                    printf("found main!\n");
                    vec_LLVMBuilderRef_push_back(
                      &b_ref,
                      add_default__start_func(
                        module,
                        f,
                        ret_type)); // the PIR should have only 1 main anyways
                }
                vec_FuncVarReg_free(&v);
            }
            vec_LLVMBuilderRef_push_back(&b_ref, builder); // list for dispose
            continue;
        }
        }
    }
    vec_LLVMBuilderRef_free(&b_ref);
    vec_Func_free(&fs);
    log_debug("successfully generate LLVM Module");
    return module;
}

void
compile_module(LLVMModuleRef module, char* name)
{
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
    LLVMTargetMachineRef machine =
      LLVMCreateTargetMachine(t,
                              triple,
                              "generic",
                              "",
                              LLVMCodeGenLevelAggressive,
                              LLVMRelocDynamicNoPic,
                              LLVMCodeModelMedium);
    // TODO: link here
    LLVMTargetMachineEmitToFile(machine, module, name, LLVMObjectFile, NULL);
    LLVMDisposeMessage(triple);
    LLVMDisposeTargetMachine(machine);
}
