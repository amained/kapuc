// for compiling the PIR
#include "PIR.h"

#include "llvm-c/Types.h"

#include <stdbool.h>

LLVMModuleRef
generate_LLVM_IR(struct PIR* p, char* module_name);
void
compile_module(LLVMModuleRef module, char* name);