#ifndef KAPUC_VERSION
#define KAPUC_VERSION "unknown (unstable)"
#endif

#include <llvm/Support/CommandLine.h>
using namespace llvm;

static cl::OptionCategory CompilerCategory("kapuc Compiler options",
                                           "Compiler arguments");

static cl::OptionCategory DebugCategory(
  "kapuc Compiler debug options",
  "Compiler arguments, specifically for debugging the compiler");

static cl::opt<std::string> input(cl::Positional,
                                  cl::desc("input file"),
                                  cl::Required,
                                  cl::cat(CompilerCategory));

static cl::opt<bool> print_ir("print-ir",
                              cl::desc("Print debug LLVM IR onto screen"),
                              cl::cat(DebugCategory));

static cl::opt<std::string> output("o",
                                   cl::desc("output"),
                                   cl::value_desc("output file"),
                                   cl::cat(CompilerCategory));

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

static cl::VersionPrinterTy kapuc_version(raw_ostream &o)  {
	o << "Kapuc version " << KAPUC_VERSION << "\n\0";
	return 0;
}

extern "C"
{
    void parse_commandline_options(int argc, char*** argv)
    {
	ArrayRef options = {&CompilerCategory, &DebugCategory};
	cl::HideUnrelatedOptions(options);
	cl::AddExtraVersionPrinter(kapuc_version);
        cl::ParseCommandLineOptions(argc, *argv, "Kapuc: Kapu Compiler");
    }
    get_opt_c_str(input) get_opt_c_str(output) get_opt_bool(print_ir)
}
