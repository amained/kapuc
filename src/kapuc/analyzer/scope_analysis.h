#include "lib/sds.h"
struct k_trail;

typedef struct block_variables
{
    sds dest;
    struct k_trail* type;
    int8_t assign_type; // 0: number
    union
    {}; // value here
} block_variables;

// i love reverse linked list
// TODO: Figure out if k_trail should be linked list or reverse linked list?
struct k_trail
{
    block_variables current;
    block_variables* prev;
}; // type trail

#define T block_variables
void
block_variables_free(T*);
T
block_variables_copy(T*);
#include "lib/ctl/vec.h"
// type of variables vector will be vec_block_variables

struct scope_datas
{
    vec_block_variables scope_variables;
    struct scope_datas*
      previous_scopes; // we use previous scope because a scope can only have 1
                       // previous scopes also with this we can get previous
                       // scope's variables to do type analysis
};

#undef T
