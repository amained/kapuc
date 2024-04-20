// env_args.h is a 1 file library to get arguments from env.
#include <stdbool.h>
#include <stdlib.h>

#ifndef CLWB_ENV_ARGS_H
#define CLWB_ENV_ARGS_H

#define env_arg_str(name, out, default)                                        \
    char* env = getenv(name);                                                  \
    if (env != NULL) {                                                         \
        out = env;                                                             \
    } else {                                                                   \
        out = default;                                                         \
    }

#define export_opt_c_str(opt_name) char* get_##opt_name();
#define export_opt_bool(opt_name) bool get_##opt_name();
#define export_opt_enum(opt_name, opt_type) opt_type get_##opt_name();
#define export_opt_bits_set(opt_name, opt_smallname)                           \
    bool get_##opt_name_##opt_smallname();

enum FOptimizeOptions
{
    opt_dce,
    opt_instsimplify,
    opt_inlining,
    opt_strip
};

enum OLevel
{
    None,
    O1,
    O2,
    O3
};

#ifndef KAPUC_VERSION // HACK, TODO: Remove this and replace with include guard
                      // or smth like that?

void
parse_commandline_options(int argc, char*** argv);
export_opt_c_str(input);
export_opt_c_str(output);
export_opt_bool(print_ir);
export_opt_enum(OptsLevel, enum OLevel);
export_opt_bits_set(OptimizationBits, opt_dce);
#endif

#undef export_opt_c_str
#undef export_opt_bool
#undef export_opt_enum
#undef export_opt_bits_set
#undef parse_commandline_options

#endif // CLWB_ENV_ARGS_H
