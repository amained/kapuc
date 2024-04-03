#include "helper.h"
#include "lib/env_args.h"
#include "lib/log.h"
#include <stdlib.h>
#include <unistd.h>

#define STB_DS_IMPLEMENTATION
#define SHIT_IS_IN_TESTING
#include "lex.h"
#include "lib/stb_ds.h"
#include "parse.h"
#include "llvm-c/Core.h"
#include "llvm-c/Target.h"
#include "llvm-c/TargetMachine.h"
#include <stdio.h>

void test_llvm_wasm() {
  LLVMContextRef c = LLVMContextCreate();
  LLVMModuleRef module = LLVMModuleCreateWithName("llvm_test_wasm_module");
  LLVMTypeRef param_types[] = {LLVMInt32Type(), LLVMInt32Type()};
  LLVMTypeRef ret_type = LLVMFunctionType(LLVMInt32Type(), param_types, 2, 0);
  LLVMValueRef sum = LLVMAddFunction(module, "add", ret_type);
  LLVMAddAttributeAtIndex(
      sum, -1, LLVMCreateStringAttribute(c, "wasm-export-name", 16, "sum", 3));

  LLVMBasicBlockRef entry = LLVMAppendBasicBlock(sum, "entry");
  LLVMBuilderRef builder = LLVMCreateBuilder();
  LLVMPositionBuilderAtEnd(builder, entry);

  LLVMValueRef tmp =
      LLVMBuildAdd(builder, LLVMGetParam(sum, 0), LLVMGetParam(sum, 1), "tmp");
  LLVMBuildRet(builder, tmp);
  LLVMDumpModule(module);

  LLVMDisposeBuilder(builder);

  LLVMInitializeAllTargetInfos();
  LLVMInitializeAllTargets();
  LLVMInitializeAllTargetMCs();
  LLVMInitializeAllAsmParsers();
  LLVMInitializeAllAsmPrinters();
  char *triple = "wasm32-unknown-wasi";
  LLVMTargetRef target = LLVMGetTargetFromName("wasm32");
  LLVMTargetMachineRef machine =
      LLVMCreateTargetMachine(target, triple, "", "", LLVMCodeGenLevelDefault,
                              LLVMRelocDynamicNoPic, LLVMCodeModelMedium);
  LLVMTargetMachineEmitToFile(machine, module, "./wasm-test/test.wasm",
                              LLVMObjectFile, NULL);
  LLVMDisposeTargetMachine(machine);
}

int main(const int argc, char **argv) {
  log_set_level(1);
  if (argc < 2) {
    log_error("Usage: %s <file>", argv[0]);
    exit(1);
  }
  bool no_parse;
  char *x;
  env_arg_str("NO_PARSE", x, 0) if (x != NULL && strcmp(x, "yes") == 0) {
    no_parse = true;
  }
  else {
    no_parse = false;
  }
  BENCH_TIMER_SETUP FILE *f = fopen(argv[1], "r");
  if (f == NULL) {
    log_error("Cannot open %s, exiting", argv[1]);
    exit(1);
  }
  log_debug("lexing started");
  BENCH_TIMER_HELPER("lexed", struct TOK *tokens = lex(f);)
  if (arrlast(tokens).t == T_ERR) {
    log_error("failed lexing");
    exit(1);
  }
  log_debug("lexing finished: got %d tokens", arrlen(tokens));
#ifdef SUPER_AGGRESSIVE_DEBUG
  for (int i = 0; i < arrlen(tokens); i++)
    log_debug("tokens %d is %d: %s (pos: %ld-%ld)", i + 1, tokens[i].t,
              tokens[i].s, tokens[i].start + 1, tokens[i].end + 1);
#endif
  if (!no_parse) {
    log_debug("parsing started");
    struct Parser p = {.tokens = tokens, .pos = 0, .filename = argv[1]};
    if (p.error) {
      log_error("failed parsing");
      exit(1);
    }
    log_debug("parsing finished");
    free_parser(&p);
    log_debug("testing llvm");
    test_llvm_wasm();
    log_debug("end llvm test");
  }

  // cleanup
  // for (int i = 0; i < arrlen(trees); i++) {
  // free_ast(&trees[i]);
  // }
  // for (int i = 0; i < arrlen(tokens); i++) {
  //    sdsfree(tokens[i].s);
  // }
  // arrfree(trees);
  fclose(f);
}
