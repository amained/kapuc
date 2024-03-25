// env_args.h is a 1 file library to get arguments from env.
#include <stdlib.h>

#ifndef CLWB_ENV_ARGS_H
#define CLWB_ENV_ARGS_H

#define env_arg_str(name, out, default) \
        char *env = getenv(name); \
        if (env != NULL) { \
            out = env; \
        } else { \
            out = default; \
        }

#endif //CLWB_ENV_ARGS_H
