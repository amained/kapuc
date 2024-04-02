def _llvm_custom_builds_impl(ctx):
    # Construct the download URL dynamically using the architecture attribute
    version = "17.x"
    
    # Determine the architecture and OS
    name = ctx.attr.name
    # Download and extract the LLVM package
    ctx.download_and_extract(
        # TODO: fix whatever is .x.zip
        url = "https://github.com/amained/llvm-custom-builds/releases/download/%s/llvm-%s-%s.zip" % (version, name, version)
    )
    
    ctx.extract(
            "llvm.tar.xz"
    )
    
    # Construct the full path to llvm-config
    llvm_config_path = "bin/llvm-config"
    
    # Execute llvm-config to get the necessary compiler flags and linker flags
    # TODO: change linkopts to include only needed flags
    copts_execute = ctx.execute([llvm_config_path, "--cflags"], quiet=False)
    linkopts_execute = ctx.execute([llvm_config_path, "--ldflags", "--system-libs", "--libs"], quiet=False)

    if copts_execute.return_code != 0 or linkopts_execute.return_code != 0:
        print("wtf")
        print(copts_execute.stderr.strip())
        print(linkopts_execute.stderr.strip())
        fail("llvm-config return non-zero exit code")

    # Decode the outputs and strip whitespace
    copts = copts_execute.stdout.strip().split(' ')
    linkopts = linkopts_execute.stdout.strip().split(' ')
    
    # Generate a BUILD file for the LLVM dependency
    ctx.file('BUILD', """
package(default_visibility = ["//visibility:public"])
cc_library(
    name = "llvm",
    srcs = glob(["lib/libLLVM.a"]),
    hdrs = glob([
        "usr/include/**/*.h",
        "usr/include/**/*.inc",
        "usr/include/**/*.def",
        "usr/include/**/*.gen",
    ]),
    includes = ["usr/include"],
    copts = %s,
    linkopts = %s,
    visibility = ["//visibility:public"],
)
""" % (copts, linkopts))
    
    ctx.file("WORKSPACE", "")

# Define the repository rule with architecture as an attribute
llvm = repository_rule(
    implementation = _llvm_custom_builds_impl,
)
