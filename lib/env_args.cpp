#ifndef KAPUC_VERSION
#define KAPUC_VERSION "unknown (unstable)"
#endif

#include "env_args.h"
#include <llvm/Support/CommandLine.h>
using namespace llvm;

// Generic compiler options
static cl::OptionCategory CompilerCategory("kapuc Compiler options",
                                           "Compiler arguments");

static cl::opt<std::string> input(cl::Positional,
                                  cl::desc("input file"),
                                  cl::Required,
                                  cl::cat(CompilerCategory));

static cl::opt<std::string> output("o",
                                   cl::desc("output"),
                                   cl::value_desc("output file"),
                                   cl::cat(CompilerCategory));
static cl::opt<OLevel> OptsLevel(
  cl::desc("Optimization Level:"),
  cl::values(clEnumValN(None, "g", "No optimization, debug info"),
             clEnumVal(O1, "No optimization"),
             clEnumVal(O2, "Default optimization"),
             clEnumVal(O3, "Aggressive optimization")),
  cl::cat(CompilerCategory));

static cl::bits<FOptimizeOptions> OptimizationBits(
  "fO",
  cl::desc("Available Optimize Options:"),
  cl::values(
    clEnumValN(opt_dce, "dce", "Dead Code Elimination"),
    clEnumValN(opt_instsimplify, "is-simplify", "Instruction Simplification"),
    clEnumValN(opt_inlining, "inline", "Procedure Integration"),
    clEnumValN(opt_strip, "strip-symbol", "Strip Symbols")),
  cl::cat(CompilerCategory),
  cl::Prefix);

// For debugging compiler
static cl::OptionCategory DebugCategory(
  "kapuc Compiler debug options",
  "Compiler arguments, specifically for debugging the compiler");

static cl::opt<bool> print_ir("print-ir",
                              cl::desc("Print debug LLVM IR onto screen"),
                              cl::cat(DebugCategory));

#define get_opt_c_str(opt_name)                                                \
    const char* get_##opt_name()                                               \
    {                                                                          \
        return opt_name.c_str();                                               \
    }

#define get_opt_bool(opt_name)                                                 \
    bool get_##opt_name()                                                      \
    {                                                                          \
        return opt_name;                                                       \
    }
#define get_opt_bits_set(opt_name, opt_smallname)                              \
    bool get_##opt_name_##opt_smallname()                                      \
    {                                                                          \
        return opt_name.isSet(opt_smallname);                                  \
    }
#define get_opt_enum(opt_name, opt_type)                                       \
    opt_type get_##opt_name()                                                  \
    {                                                                          \
        return opt_name;                                                       \
    }

static cl::VersionPrinterTy
kapuc_version(raw_ostream& o)
{
    o << "Kapuc version " << KAPUC_VERSION << "\n\0";
    return 0;
}

extern "C"
{
    void parse_commandline_options(int argc, char*** argv)
    {
        ArrayRef options = { &CompilerCategory, &DebugCategory };
        cl::HideUnrelatedOptions(options);
        cl::AddExtraVersionPrinter(kapuc_version);
        cl::ParseCommandLineOptions(argc, *argv, "Kapuc: Kapu Compiler");
    }
    get_opt_c_str(input);
    get_opt_c_str(output);
    get_opt_bool(print_ir);
    get_opt_enum(OptsLevel, OLevel);
    get_opt_bits_set(OptimizationBits, opt_dce);
}
