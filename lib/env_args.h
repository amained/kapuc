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

void
parse_commandline_options();

export_opt_c_str(input) export_opt_c_str(output) export_opt_bool(print_ir)

#undef export_opt_c_str
#undef export_opt_bool

#endif // CLWB_ENV_ARGS_H
