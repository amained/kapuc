#include "helper.h"
#include "lib/log.h"

#include <stdlib.h>
#include <unistd.h>

#define STB_DS_IMPLEMENTATION
#define SHIT_IS_IN_TESTING
#include "analyzer/analysis.h"
#include "lex.h"
#include "lib/env_args.h"
#include "lib/stb_ds.h"
#include "parse.h"

#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"

#include <stdio.h>

// Testing LLVM, incase something f'ed up

void
test_llvm_wasm()
{
    LLVMContextRef c = LLVMContextCreate();
    LLVMModuleRef module = LLVMModuleCreateWithName("llvm_test_wasm_module");
    LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
    LLVMValueRef sum = LLVMAddFunction(module, "add", ret_type);
    LLVMSetLinkage(sum, LLVMExternalLinkage);
    LLVMAddAttributeAtIndex(
      sum,
      -1,
      LLVMCreateStringAttribute(
        c,
        "wasm-export-name",
        16,
        "sum",
        3)); // Wasm export thing, https://reviews.llvm.org/D70520 (old LLVM's
             // phabricator thing) Maybe we could do more stuff with this
             // (export alias in WASM?)

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);

    LLVMValueRef tmp =
      LLVMBuildAdd(builder, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1), "tmp");
    LLVMBuildRet(builder, tmp);
    if (get_print_ir())
        LLVMDumpModule(module);

    char* triple = "wasm32-unknown-unknown";
    LLVMTargetRef target = LLVMGetTargetFromName("wasm32");
    LLVMTargetMachineRef machine =
      LLVMCreateTargetMachine(target,
                              triple,
                              "",
                              "",
                              LLVMCodeGenLevelAggressive,
                              LLVMRelocDynamicNoPic,
                              LLVMCodeModelMedium);
    LLVMTargetMachineEmitToFile(
      machine, module, "./wasm-test/test.wasm", LLVMObjectFile, NULL);
    LLVMDisposeTargetMachine(machine);
    LLVMDisposeBuilder(builder);
    LLVMContextDispose(
      c); // what is this consistency, also this needs to be
          // disposed AFTER LLVMTargetMachineEmitToFile() or else
          // we won't have the context in the module I think?
}

void
test_llvm_native()
{
    LLVMModuleRef module = LLVMModuleCreateWithName("llvm_test_native");
    LLVMTypeRef param_types[] = { LLVMInt32Type(), LLVMInt32Type() };
    LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
    LLVMValueRef sum = LLVMAddFunction(module, "add", ret_type);

    LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");
    LLVMBuilderRef builder = LLVMCreateBuilder();
    LLVMPositionBuilderAtEnd(builder, entry);
    LLVMValueRef tmp =
      LLVMBuildAdd(builder, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1), "tmp");
    LLVMBuildRet(builder, tmp);

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
    LLVMValueRef arg[] = { LLVMConstInt(LLVMInt32Type(), 0, 0) };
    LLVMBuildCall2(builder2, exit_type, exit_func, arg, 1, "");
    LLVMBuildUnreachable(builder2);
    if (get_print_ir())
        LLVMDumpModule(module);

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
      LLVMCodeModelMedium); // this should not error? TODO: maybe check for
                            // NULL?
    // NOTE: This does not link! This just build object file (ELF in linux (and
    // *nix?), idk what in Windows) To link on linux, specify -lc and
    // -dynamic-linker to ld
    // TODO: link it with LLVM
    LLVMTargetMachineEmitToFile(
      machine, module, "test.o", LLVMObjectFile, NULL);
    LLVMDisposeMessage(triple);
    LLVMDisposeTargetMachine(machine);
    LLVMDisposeBuilder(builder);
    LLVMDisposeBuilder(builder2);
}

int
main(const int argc, char** argv)
{
    log_set_level(1);
    if (argc < 2) {
        log_error("Usage: %s <file>", argv[0]);
        exit(1);
    }
    parse_commandline_options(argc, &argv);
    log_debug("Optimization level: %d", get_OptsLevel());
    char* input = get_input();
    log_debug("Input file: %s", input);
    bool no_parse;
    char* x;
    env_arg_str("NO_PARSE", x, 0);
    if (x != NULL && strcmp(x, "yes") == 0)
        no_parse = true;
    else
        no_parse = false;

    BENCH_TIMER_SETUP FILE* f = fopen(input, "r");
    if (f == NULL) {
        log_error("Cannot open %s, exiting", argv[1]);
        exit(1);
    }
    log_debug("lexing started");
    BENCH_TIMER_HELPER("lexed", struct TOK* tokens = lex(f);)
    if (arrlast(tokens).t == T_ERR) {
        log_error("failed lexing");
        exit(1);
    }
    log_debug("lexing finished: got %d tokens", arrlen(tokens));
#ifdef SUPER_AGGRESSIVE_DEBUG
    for (int i = 0; i < arrlen(tokens); i++) {
        log_debug("tokens %d is %d: %s (pos: %ld-%ld)",
                  i + 1,
                  tokens[i].t,
                  tokens[i].s,
                  tokens[i].start + 1,
                  tokens[i].end + 1);
    }

#endif
    if (!no_parse) {
        log_debug("parsing started");
        struct parser p = { tokens, 0 };
        struct parse_tree* tree = malloc(sizeof(struct parse_tree));
        if (!build_block(&p, tree))
            log_debug("we fucked");
        else {
            log_debug("parsing finished");
            print_entire_expression(tree);
            putchar('\n');
            log_debug("tree type: %d", tree->type);
            // try checking block
            if (tree->type == LVL_STMTS) {
                log_debug("got lvl_stmts, try checking block");
                check_block(tree, NULL);
                log_debug("ooh we survive segfault");
            }
            free_parse_tree(tree);
        }
    }
    log_debug("testing llvm");
    // initialize all
    LLVMInitializeAllTargetInfos();
    LLVMInitializeAllTargets();
    LLVMInitializeAllTargetMCs();
    LLVMInitializeAllAsmParsers();
    LLVMInitializeAllAsmPrinters();

    test_llvm_wasm();
    test_llvm_native();
    LLVMShutdown(); // Somehow after disposing all, we need to shutdown (kind of
                    // make sense because LLVMInitialize*() stuff but very
                    // beautiful api design)
    log_debug("end llvm test");

    // cleanup
    // for (int i = 0; i < arrlen(trees); i++) {
    // free_ast(&trees[i]);
    // }
    for (int i = 0; i < arrlen(tokens); i++)
        sdsfree(tokens[i].s);
    arrfree(tokens);
    // arrfree(trees);
    fclose(f);
}
