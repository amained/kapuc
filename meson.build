project('kapuc', 'c', 'cpp', default_options : ['c_std=c18', 'cpp_std=c++2a'])

# Define the LLVM dependency
llvm_dep = dependency('llvm', method: 'auto', version: '>= 17.0', modules: ['core', 'support'])

# Define the source files and include directories for each library
sds_sources = files('lib/sds.c')
sds_headers = files('lib/sds.h', 'lib/sdsalloc.h')

ctl_sources = files('lib/ctl/ctl.h', 'lib/ctl/vec.h')

log_sources = files('lib/log.c')
log_headers = files('lib/log.h')

env_args_sources = files('lib/env_args.cpp')
env_args_headers = files('lib/env_args.h')

kapuc_sources = files('src/kapuc/helper.h', 'src/kapuc/lex.c', 'src/kapuc/lex.h', 'src/kapuc/main.c', 'src/kapuc/parse.c', 'src/kapuc/parse.h', 'src/kapuc/analyzer/analysis.c', 'src/kapuc/analyzer/analysis.h', 'src/kapuc/analyzer/scope_analysis.h')

# Define the libraries
sds_lib = static_library('sds', sds_sources, include_directories: include_directories('.'))
ctl_lib = library('ctl', ctl_sources, include_directories: include_directories('.'))
log_lib = static_library('log', log_sources, include_directories: include_directories('.'))
env_args_lib = static_library('env_args', env_args_sources, include_directories: include_directories('.'), dependencies: [llvm_dep])

# Define the binary
kapuc_bin = executable('kapuc', kapuc_sources, dependencies: [llvm_dep], link_with: [sds_lib, ctl_lib, log_lib, env_args_lib])

# Install targets
install_dir = join_paths(get_option('prefix'), 'bin')

