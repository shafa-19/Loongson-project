
# -*- coding: utf-8 -*-
"""make_project.py -- builds a LoongIDE project tree exactly as the New Project
Wizard does (recipe read from D:\LoongIDE\Template\Template.ini), then writes a
working .lxp project file.

usage:  python make_project.py <target_dir> <project_name> [bare|rtthread]
"""
import os, shutil, sys, re

TPL = r"D:\LoongIDE\Template"
LS1B_EXAMPLE = r"D:\loongson1B_example\example_6_1\example_6_1.lxp"

RTTHREAD_DIRS = [
    ("RTThread\\src",           "RTT4\\src"),
    ("RTThread\\include",       "RTT4\\include"),
    ("RTThread\\port",          "RTT4\\port"),
    ("RTThread\\components",    "RTT4\\components"),
    ("general\\mips",           "RTT4\\port\\mips"),
    ("ls1b\\mcuinclude",        "RTT4\\port\\include"),
    ("RTThread\\bsp-ls1x",      "RTT4\\bsp-ls1x"),
    ("general\\ls1x-drv",       "ls1x-drv"),
    ("ls1b\\include",           "include"),
]
RTTHREAD_FILES = [
    ("RTThread\\ls1b\\ld.script",  "ld.script"),
    ("RTThread\\ls1b\\rtconfig.h", "include\\rtconfig.h"),
    ("libraries\\install_3th_libraries.c", "src\\install_3th_libraries.c"),
]
RTTHREAD_INC = [
    "./RTT4/include", "./RTT4/port/include", "./RTT4/port/mips",
    "./RTT4/components/finsh", "./RTT4/components/dfs/include",
    "./RTT4/components/drivers/include", "./RTT4/components/libc/time",
    "./RTT4/bsp-ls1x", "./ls1x-drv/include", "./include",
]

SKIP_EXT = (".lxp", ".layout", ".o", ".d", ".exe", ".bin", ".bak")


def copytree(src, dst):
    if not os.path.isdir(src):
        raise SystemExit("missing template dir: " + src)
    shutil.copytree(src, dst, dirs_exist_ok=True)


def build_tree(target, kind):
    os.makedirs(target, exist_ok=True)
    if kind != "rtthread":
        raise SystemExit("only rtthread implemented")
    for s, d in RTTHREAD_DIRS:
        copytree(os.path.join(TPL, s), os.path.join(target, d))
    for s, d in RTTHREAD_FILES:
        dst = os.path.join(target, d)
        os.makedirs(os.path.dirname(dst), exist_ok=True)
        shutil.copy2(os.path.join(TPL, s), dst)
    # the wizard copies one demo driver set; we only need the uart/gpio/pin drivers
    src = os.path.join(TPL, "demo", "rtthread-1b", "main.c")
    if os.path.isfile(src):
        shutil.copy2(src, os.path.join(target, "main.c"))


def scan(target):
    folders, units = [], []
    for root, dirs, files in os.walk(target):
        dirs[:] = sorted(d for d in dirs if d != "Build")
        rel = os.path.relpath(root, target).replace("\\", "/")
        if rel != ".":
            folders.append(rel)
        for f in sorted(files):
            if f.endswith(SKIP_EXT):
                continue
            units.append((f, "" if rel == "." else rel))
    return sorted(folders), units


def make_lxp(target, name, kind):
    base = open(LS1B_EXAMPLE, encoding="utf-8", errors="replace").read()

    def section(txt, header):
        m = re.search(r"^\[" + header + r"\]\r?\n(.*?)(?=^\[|\Z)", txt, re.S | re.M)
        return m.group(1) if m else ""

    def set_field(block, key, value):
        if re.search(r"^" + key + r"=.*$", block, re.M):
            return re.sub(r"^" + key + r"=.*$", lambda m: key + "=" + value, block, flags=re.M)
        return block + key + "=" + value + "\n"

    folders, units = scan(target)

    proj = section(base, "Project")
    proj = set_field(proj, "filename", name + ".lxp")
    proj = set_field(proj, "name", name)
    proj = set_field(proj, "FoldersCount", str(len(folders)))
    proj = set_field(proj, "UnitCount", str(len(units)))
    proj = set_field(proj, "CompilerSet", "SDE Lite 4.9.2 for MIPS")
    proj = set_field(proj, "RTOSName", "RT-Thread")
    proj = set_field(proj, "RTOSVer", "4.0.2")
    proj = set_field(proj, "PortVer", "1.0.0")
    proj = set_field(proj, "DriverVer", "1.0.0")

    cc = section(base, "CCompiler")
    cc = set_field(cc, "GxxFileName", r"D:\LoongIDE\mips-2015.05\bin\mips-sde-elf-gcc.exe")
    cc = set_field(cc, "IncludePaths", ";".join(RTTHREAD_INC))
    cc = set_field(cc, "DefinedSymbols", "LS1B;OS_RTTHREAD")
    cc = set_field(cc, "GxxFlags", "-mips32 -G0 -EL -msoft-float -DLS1B -DOS_RTTHREAD  -O0 -g -Wall -c -fmessage-length=0 -pipe")

    lnk = section(base, "Linker")
    lnk = set_field(lnk, "GxxFileName", r"D:\LoongIDE\mips-2015.05\bin\mips-sde-elf-gcc.exe")

    arc = section(base, "Archiver")
    arc = set_field(arc, "GxxFileName", r"D:\LoongIDE\mips-2015.05\bin\mips-sde-elf-ar.exe")

    dbg = section(base, "Debug")
    dbg = set_field(dbg, "AutoRebuild", "0")
    dbg = set_field(dbg, "HasPMON", "1")
    dbg = set_field(dbg, "DownloadByNet", "1")
    dbg = set_field(dbg, "ResetAtStart", "1")
    dbg = set_field(dbg, "SetBPAtFunc", "")

    dbgr = "[Debugger]\nCount=0\n\n"

    out = []
    out.append("[Project]\n" + proj)
    out.append("[McuAndBSP]\n" + section(base, "McuAndBSP"))
    out.append("[CCompiler]\n" + cc)
    out.append("[CppCompiler]\n" + section(base, "CppCompiler"))
    out.append("[Assembler]\n" + section(base, "Assembler"))
    out.append("[Linker]\n" + lnk)
    out.append("[Archiver]\n" + arc)
    out.append("[Debug]\n" + dbg)
    out.append(dbgr)
    f = ["[Folders]"]
    for i, d in enumerate(folders, 1):
        f.append("Folders%d=%s" % (i, d))
    out.append("\n".join(f) + "\n\n")
    for i, (fn, folder) in enumerate(units, 1):
        out.append("[Unit%d]\nFileName=%s\nFolder=%s\n\n" % (i, fn, folder))

    path = os.path.join(target, name + ".lxp")
    with open(path, "w", encoding="utf-8", newline="\r\n") as fh:
        fh.write("".join(out))
    print("lxp written:", path)
    print("folders:", len(folders), "units:", len(units))


def main():
    if len(sys.argv) < 3:
        raise SystemExit(__doc__)
    target, name = sys.argv[1], sys.argv[2]
    kind = sys.argv[3] if len(sys.argv) > 3 else "rtthread"
    build_tree(target, kind)
    make_lxp(target, name, kind)


if __name__ == "__main__":
    main()
