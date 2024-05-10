// for generation of the PIR
#include <stdbool.h>
#include "PIR.h"

bool print_PIR(struct PIR* p);

struct PIR* create_PIR(); // return 0 if failed
void free_PIR(struct PIR* p);

size_t add_function_to_PIR(struct PIR* p, sds function_name, typing *t); // return the index of the PIR, -1 if messed up
size_t add_block_to_function(struct PIR* p, int func_index); // return the index of the block, -1 if messed up

size_t
add_Expr_to_block(struct PIR* p,
                 int func_index,
                 int block_index,
                 expr* value,
                 FUNC_VAR v);
size_t
add_Ret_to_block(struct PIR* p,
                 int func_index,
                 int block_index,
                 expr* value);
#define INT8_TYPING {.is_default_type = true, .default_type = 0}
#define INT16_TYPING {.is_default_type = true, .default_type = 1}