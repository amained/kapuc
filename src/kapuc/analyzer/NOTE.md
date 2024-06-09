# Analyzer
- this is also supposed to generate something around the line of LLVM IR and the structure of the code (like Rust's MIR)
- at least after we do all the type check
- so we need to keep track of
  - the code in current block (that will store the block under it)
- types imported resolves from top to bottom
- scope resolve orders are like this (files are supposed to be like, only represented in the codegen (as 0, to be exact), not usable in real code)
  - types
    (file::) types -> imported :: types -> std:: types
  - function
    (file::) function -> imported :: functions -> std:: functions
  - variables
    this just resolves from outer block to inner block.
    (currently) there is no constant defined for entire file.
    if it exist it would probably exist as the biggest file:: scope

## The PIR (Poor man's IR)
- so we reduced down the syntatic sugar from
  - Normal blocks to basic block (CFG)
  - Variables are changed such that
    - they have no name (are represented by the index on the scope_variables on debug)
    - they are in static single assignment form (SSA)
  - function calls are just jump to block
- some example
  - assignment
  ```
  {
    let i8 x = 0;
    x = 1;
  }
  ```
  is
  ```
  {
    _1 = 0;
    _2 = 1;
  }
  ```

  (the reason this should work is that we check for types and constant first before generating the PIR, if we don't do that it would fails)
  (the first assignment will be marked useless)

  - using itself
  ```
  {
    let i8 x = 0;
    x = x + 1;
  }
  ```
  is
  ```
  $start {
    _1 = 0;
    _2 = _1 + 1;
  }
  ```

  (the assignment should be merged by the optimizer since the first assignment is useless)

  - if-else/multiple block
  ```
  {
    const i8 test = 1;
    if test == 1 return 0;
    else return 1;
  }
  ```
  is
  ```
    {
      i8 _1;
      $0 {
        _1 = 1;
        IC(_1, 1: $0, _: $1)
      }
      $1 {
        RET(0);
      }
      $2 {
        RET(1);
      }
    }
  ```
  
  (IC are int case, RET are for return, I am not so sure if you could just return without creating variable to return in LLVM?)

- Definition of PIR (in some weird CFG-or-struct-like form?)
  ```
  FUNC = VAR* BLOCK* // we will use $0 block as the start of function
  VAR = {TYPE ID}
  BLOCK = {ID STMTS}
  STMTS = ASSIGNMENT | RET | IC
  ASSIGNMENT = {ID VAL}
  VAL = NUMBER | BOOL // would this need more data type?
  RET = {VAL}
  IC = {VAL COND* _COND} // default condition is _
  COND = {VAL JMP}
  ```