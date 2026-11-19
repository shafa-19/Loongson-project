# -*- coding: utf-8 -*-
"""build_proj.py -- builds a LoongIDE project without the IDE's makefiles.

Compiles every .c / .S of the project (Build/ excluded) with the SDE Lite
toolchain, then links with the project's ld.script and produces a .bin.

usage: python build_proj.py <project_dir> <project_name> [-D EXTRA ...]
"""
import os, subprocess, sys, concurrent.futures

TC = r"D:\LoongIDE\mips-2015.05\bin"
CC = os.path.join(TC, "mips-sde-elf-gcc.exe")
OBJCOPY = os.path.join(TC, "mips-sde-elf-objcopy.exe")
SIZE = os.path.join(TC, "mips-sde-elf-size.exe")


def main():
    proj = os.path.abspath(sys.argv[1])
    name = sys.argv[2]
    extra = sys.argv[3:]

    incs = [
        "RTT4/include", "RTT4/port/include", "RTT4/port/include/mcuinclude",
        "RTT4/port/mips", "RTT4/components/finsh", "RTT4/components/dfs/include",
        "RTT4/components/drivers/include", "RTT4/components/libc/time",
        "RTT4/bsp-ls1x", "ls1x-drv/include", "include",
        "core/include", "core/mips",
        # lwIP (ignored when the folder is absent)
        "lwIP-1.4.1/include", "lwIP-1.4.1/include/ipv4", "lwIP-1.4.1/port/include",
    ]
    incflags = ["-I" + os.path.join(proj, p) for p in incs
                if os.path.isdir(os.path.join(proj, p))]

    # OS_RTTHREAD only for RT-Thread projects: the bare-metal BSP takes the
    # other branch of its #ifdefs (no rtconfig.h / rtthread.h there).
    # The BSP selects its OS branch with BSP_USE_OS = (!defined(OS_NONE)), so a
    # bare-metal project must be compiled with -DOS_NONE (this is exactly what
    # the IDE puts into GxxFlags of a "Bare Program" project).
    osflags = (["-DOS_RTTHREAD"] if os.path.isdir(os.path.join(proj, "RTT4"))
               else ["-DOS_NONE"])

    cflags = ["-mips32", "-G0", "-EL", "-msoft-float", "-DLS1B"] + osflags +              ["-O1", "-g", "-Wall", "-c", "-fmessage-length=0", "-pipe"] + incflags + extra

    srcs = []
    for root, dirs, files in os.walk(proj):
        dirs[:] = [d for d in dirs if d != "Build"]
        for f in sorted(files):
            if f.endswith((".c", ".S")):
                srcs.append(os.path.join(root, f))

    objdir = os.path.join(proj, "Build", "obj")
    os.makedirs(objdir, exist_ok=True)

    def build_one(src):
        rel = os.path.relpath(src, proj)
        obj = os.path.join(objdir, rel.replace("\\", "_").replace("/", "_") + ".o")
        r = subprocess.run([CC] + cflags + ["-o", obj, src],
                           capture_output=True, text=True)
        return (src, obj, r.returncode, (r.stderr or "")[:400])

    objs, failed = [], []
    with concurrent.futures.ThreadPoolExecutor(max_workers=8) as ex:
        for src, obj, rc, err in ex.map(build_one, srcs):
            if rc != 0:
                failed.append((src, err))
            else:
                objs.append(obj)

    print("compiled: %d / %d" % (len(objs), len(srcs)))
    if failed:
        for src, err in failed[:6]:
            print("FAILED:", src)
            print("   ", err.replace("\n", "\n    ")[:300])
        return 1

    exe = os.path.join(proj, "Build", name + ".exe")
    bin_ = os.path.join(proj, "Build", name + ".bin")
    link = ([CC, "-mips32", "-G0", "-EL", "-msoft-float", "-nostartfiles", "-static",
             "-T", os.path.join(proj, "ld.script"), "-o", exe, "-e", "start"]
            + objs + ["-Wl,--start-group", "-lm", "-lc", "-lgcc", "-Wl,--end-group"])
    r = subprocess.run(link, capture_output=True, text=True)
    if r.returncode != 0:
        print("LINK FAILED")
        print((r.stderr or "")[:1500])
        return 1

    subprocess.run([OBJCOPY, "-O", "binary", exe, bin_], check=True)
    s = subprocess.run([SIZE, exe], capture_output=True, text=True)
    print(s.stdout.strip())
    print("built:", exe)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
