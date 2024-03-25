#include "helper.h"
#include "lib/env_args.h"
#include "lib/log.h"
#include <stdlib.h>
#include <unistd.h>

#define STB_DS_IMPLEMENTATION
#define SHIT_IS_IN_TESTING
#include "code_tree.h"
#include "lex.h"
#include "lib/stb_ds.h"
#include "libgccjit.h"
#include "parse.h"
#include <stdio.h>

#define gcc_jit_check_for_null(code, varname, errmesg, ctxt, after_error)      \
  varname = code;                                                              \
  if (varname == NULL) {                                                       \
    log_debug(errmesg, gcc_jit_context_get_last_error(ctxt));                  \
    after_error;                                                               \
  }

static void create_code(gcc_jit_context *ctxt, gcc_jit_function **greet) {
  /* Let's try to inject the equivalent of:
     void
     greet (const char *name)
     {
        printf ("hello %s\n", name);
     }
  */
  gcc_jit_type *void_type = gcc_jit_context_get_type(ctxt, GCC_JIT_TYPE_VOID);
  gcc_jit_type *const_char_ptr_type =
      gcc_jit_context_get_type(ctxt, GCC_JIT_TYPE_CONST_CHAR_PTR);
  gcc_jit_param *param_name =
      gcc_jit_context_new_param(ctxt, NULL, const_char_ptr_type, "name");
  *greet = gcc_jit_context_new_function(ctxt, NULL, GCC_JIT_FUNCTION_EXPORTED,
                                        void_type, "greet", 1, &param_name, 0);

  gcc_jit_param *param_format =
      gcc_jit_context_new_param(ctxt, NULL, const_char_ptr_type, "format");
  gcc_jit_function *printf_func = gcc_jit_context_new_function(
      ctxt, NULL, GCC_JIT_FUNCTION_IMPORTED,
      gcc_jit_context_get_type(ctxt, GCC_JIT_TYPE_INT), "printf", 1,
      &param_format, 1);
  gcc_jit_rvalue **args = NULL;
  arrput(args, gcc_jit_context_new_string_literal(ctxt, "hello %s\n"));
  arrput(args, gcc_jit_param_as_rvalue(param_name));

  gcc_jit_block *block =
      gcc_jit_function_new_block(*greet, "kapuc_block_greet");

  gcc_jit_block_add_eval(
      block, NULL, gcc_jit_context_new_call(ctxt, NULL, printf_func, 2, args));
  gcc_jit_block_end_with_void_return(block, NULL);
}

static void generate_main(gcc_jit_context *ctxt, gcc_jit_function *greet_func) {
  gcc_jit_type *void_type = gcc_jit_context_get_type(ctxt, GCC_JIT_TYPE_VOID);
  gcc_jit_type *int_type = gcc_jit_context_get_type(ctxt, GCC_JIT_TYPE_INT8_T);
  gcc_jit_function *main_func = gcc_jit_context_new_function(
      ctxt, NULL, GCC_JIT_FUNCTION_EXPORTED, void_type, "main", 0, NULL, 0);
  gcc_jit_block *block =
      gcc_jit_function_new_block(main_func, "kapuc_block_main");
  // greet call
  gcc_jit_rvalue *args_greet[] = {
      gcc_jit_context_new_string_literal(ctxt, "world")};
  gcc_jit_block_add_eval(
      block, NULL,
      gcc_jit_context_new_call(ctxt, NULL, greet_func, 1, args_greet));
  // exit call
  gcc_jit_param *param_exit =
      gcc_jit_context_new_param(ctxt, NULL, int_type, "exit");
  gcc_jit_function *exit_func = gcc_jit_context_new_function(
      ctxt, NULL, GCC_JIT_FUNCTION_IMPORTED,
      gcc_jit_context_get_type(ctxt, GCC_JIT_TYPE_VOID), "exit", 1, &param_exit,
      0);
  gcc_jit_rvalue *args[] = {
      gcc_jit_context_new_rvalue_from_int(ctxt, int_type, 0)};
  gcc_jit_block_add_eval(
      block, NULL, gcc_jit_context_new_call(ctxt, NULL, exit_func, 1, args));
  gcc_jit_block_end_with_void_return(block, NULL);
}

int idek() {

  gcc_jit_timer *timer = gcc_jit_timer_new();
  if (!timer) {
    log_error("gcc_jit_timer_new failed");
    return 1;
  }
  gcc_jit_context *ctxt;

  /* Get a "context" object for working with the library.  */
  ctxt = gcc_jit_context_acquire();
  if (!ctxt) {
    log_debug("failed to acquire context: %s",
              gcc_jit_context_get_last_error(ctxt));
    return 1;
  }

  gcc_jit_context_set_timer(ctxt,
                            timer); // TODO: Check for LIBGCCJIT_HAVE_TIMING_API
  /* Set some options on the context.
     Let's see the code being generated, in assembler form.  */
  gcc_jit_context_set_bool_option(ctxt, GCC_JIT_BOOL_OPTION_DUMP_GENERATED_CODE,
                                  1);
  gcc_jit_context_set_int_option(ctxt, GCC_JIT_INT_OPTION_OPTIMIZATION_LEVEL,
                                 3);
  FILE *f = fopen("gccjit.log", "w+");
  if (f == NULL) {
    log_debug("failed to open log, not setting up log i guess");
  } else {
    gcc_jit_context_set_logfile(ctxt, f, 0, 0);
  }
  gcc_jit_timer_push(timer, "create_code");
  /* Populate the context.  */
  gcc_jit_function *func_ptr = NULL;
  create_code(ctxt, &func_ptr);
  if (func_ptr == NULL) {
    log_debug("failed to create code");
    return 1;
  }
  generate_main(ctxt, func_ptr);
  gcc_jit_timer_pop(timer, "create_code");

  gcc_jit_timer_push(timer, "compile");
  /* Compile the code.  */
  gcc_jit_context_compile_to_file(ctxt, GCC_JIT_OUTPUT_KIND_EXECUTABLE,
                                  "./main.out");
  gcc_jit_timer_pop(timer, "compile");

  /* Dump C-like shit */
  gcc_jit_context_dump_to_file(ctxt, "./main.dump", false);

  gcc_jit_timer_print(timer, stderr);
  gcc_jit_timer_release(timer);

  gcc_jit_context_release(ctxt);
  return 0;
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
  BENCH_TIMER_SETUP
  FILE *f = fopen(argv[1], "r");
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
    BENCH_TIMER_HELPER("parsed", struct AST tree = parse_func(&p);)
    if (p.error) {
      log_error("failed parsing");
      exit(1);
    }
    log_debug("parsing finished");
#ifdef SUPER_AGGRESSIVE_DEBUG
    print_tree(&tree, 0);
#endif
    log_debug("codegen started");
    BENCH_TIMER_HELPER("treegen'ed", struct CodeTreeNode code_tree =
                                         generate_function(&tree);)
    log_debug("codegen finished");
    print_tree_node(&code_tree, 0);

    free_parser(&p);
    free_tree(&tree);
  }
  // cleanup
  // for (int i = 0; i < arrlen(trees); i++) {
  // free_ast(&trees[i]);
  // }
  // for (int i = 0; i < arrlen(tokens); i++) {
  //    sdsfree(tokens[i].s);
  // }
  // arrfree(trees);
  log_debug("gcc wut");
  idek();
  log_debug("wut ended");
  fclose(f);
}
