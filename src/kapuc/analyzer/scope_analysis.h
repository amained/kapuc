#include "lib/sds.h"
struct k_trail;

typedef struct block_variables
{
    sds dest;
    struct k_trail* type;
    int8_t assign_type; // 0: number
    union
    {}; // value here (expression?)
} block_variables;

// i love reverse linked list
// TODO: Figure out if k_trail should be linked list or reverse linked list?
// current choice is to have the biggest trail be std:: (standard libraries) and
// file:: also FIXME: move this to analyzer/types.h, that should be where the
// type trail exist.
// also FIXME: this shouldn't work??
struct k_trail
{
    block_variables current;
    block_variables* prev;
    int8_t type; // from trail_type in TREE_TYPE_TRAIL, 0: atom.atom, 1:
                 // atom::atom, others are reserved
};

#define T block_variables
void
block_variables_free(T*);
T
block_variables_copy(T*);
#include "lib/ctl/vec.h"

enum statement_type
{
    CREATE_VARIABLE,
    ASSIGN_VARIABLE,
};

struct CREATE_VARIABLE_stmt
{
    sds name;
    struct k_trail type;
    // TODO: expression?
};

// pretty much block
struct scope_datas
{
    vec_block_variables scope_variables;
    struct scope_datas*
      previous_scopes; // we use previous scope because a scope can only have 1
                       // previous scopes also with this we can get previous
                       // scope's variables to do type analysis
                       // also we don't care about the inner scope once the
                       // analysis are complete (unless we don't free it then
                       // fuck)
};
