#include "lib/sds.h"

#include <stdbool.h>
#include <stdint.h>

#ifndef PIR_H
#define PIR_H

enum expr_type
{
    // operation (all are non-checked operation on LLVM)
    // TODO: add flag to change this
    Add,
    Mul,
    Del,
    Div,
    Func_val,
    Val,
};

typedef struct
{
    bool is_default_type;
    bool is_ptr;
    union
    {
        uint8_t
          default_type; // 0: int8, 1: int16, 2: int32, 3: int64, 4: bool (int1)
        // FIXME: add type trail for custom type
    };
} typing;

typedef struct
{
    typing t;
    union
    {
        int int__val;
        // FIXME: add value for custom type (struct, etc)
    };
} val;

typedef struct expr_
{
    enum expr_type t;
    union
    {
        struct
        {
            struct expr_* lhs;
            struct expr_* rhs;
        } b;
        val v;
        size_t func_val;
    };
} expr;

#define T expr
void
expr_free(T* v);
T
expr_copy(T* v);
#include "lib/ctl/vec.h"
#undef T

struct assignment
{
    int id;
    expr e;
};

typedef enum
{
    assignment,
    ret,
    call,
    ic,
} stmt_type;

typedef struct
{
    size_t call_ids; // the main_blocks index
    vec_expr value;
} call_expr;

typedef struct
{
    int jmp_case;
    int jmp_result;
} int_case;

#define T int_case
void
int_case_free(T*);
T
int_case_copy(T*);
#include "lib/ctl/vec.h"

typedef struct
{
    expr to_switch;
    vec_int_case ics;
} int_switch;

typedef struct
{
    stmt_type t;
    union
    {
        struct assignment assignment;
        expr ret_val;
        call_expr call_ca;
        int_switch ics;
    };
} stmt;
#define T stmt
void
stmt_free(T*);
T
stmt_copy(T*);
#include "lib/ctl/vec.h"

typedef typing FUNC_VAR;
#define T FUNC_VAR
void
FUNC_VAR_free(T*);
T
FUNC_VAR_copy(T*);
#include "lib/ctl/vec.h"

typedef vec_stmt BLOCK;
#define T BLOCK
void
BLOCK_free(T*);
T
BLOCK_copy(T*);
#include "lib/ctl/vec.h"

struct FUNC
{
    sds name;
    vec_FUNC_VAR vv;
    vec_BLOCK bs;
    typing t;
    bool is_external;
    bool is_variadic;
};

typedef enum
{
    func
} main_type;

typedef struct
{
    main_type type;
    union
    {
        struct FUNC func;
    };
} MAIN_BLOCK;

#define T MAIN_BLOCK
void
MAIN_BLOCK_free(T*);
T
MAIN_BLOCK_copy(T*);
#include "lib/ctl/vec.h"

// entire PIR for the file generation
struct PIR
{
    vec_MAIN_BLOCK main_blocks;
};

#endif