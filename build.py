#!/usr/bin/env python3
import os
import subprocess
import sys

def run(cmd, cwd=None):
    print(f"> {' '.join(cmd)}")
    subprocess.check_call(cmd, cwd=cwd)

def main(build_dir, build_type, generator=None, parallel=None):
    root = os.path.abspath(os.path.dirname(__file__))

    # 1. Configure
    cfg_cmd = ["cmake", "-B", build_dir, "-DCMAKE_BUILD_TYPE=" + build_type]
    if generator:
        cfg_cmd += ["-G", generator]
    cfg_cmd.append(root)
    run(cfg_cmd)

    # 2. Build
    build_cmd = ["cmake", "--build", build_dir]
    if parallel:
        build_cmd += ["--parallel", str(parallel)]
    run(build_cmd)

if __name__ == "__main__":
    import argparse
    p = argparse.ArgumentParser(description="Build JUCE plugin via CMake")
    p.add_argument("--build-dir", default="build", help="Directory for CMake build")
    p.add_argument("--config", "-c", default="Release", choices=["Debug","Release","RelWithDebInfo"], help="Build configuration")
    p.add_argument("--generator", "-G", help="CMake generator (e.g. Ninja, Xcode, \"Visual Studio 17 2022\")")
    p.add_argument("--parallel", "-j", type=int, help="Parallel build jobs")
    args = p.parse_args()
    main(args.build_dir, args.config, args.generator, args.parallel)
