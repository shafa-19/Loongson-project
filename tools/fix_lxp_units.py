# -*- coding: utf-8 -*-
"""fix_lxp_units.py -- rewrite [Folders]/[UnitN] of a LoongIDE .lxp file so that
it matches the files that really exist in the project directory.

usage: python fix_lxp_units.py <project_dir> <lxp_name>
"""
import os, re, sys

BS = chr(92)
proj = sys.argv[1]
lxp = os.path.join(proj, sys.argv[2])
s = open(lxp, encoding="utf-8", errors="replace").read()

SKIP = (".lxp", ".layout", ".o", ".d", ".exe", ".bin", ".bak", ".log")
folders, units = [], []
for root, dirs, files in os.walk(proj):
    dirs[:] = sorted(d for d in dirs if d not in ("Build", "_obj"))
    rel = os.path.relpath(root, proj).replace(BS, "/")
    if rel != ".":
        folders.append(rel)
    for f in sorted(files):
        if f.endswith(SKIP):
            continue
        units.append((f, "" if rel == "." else rel))

head = s.split("[Folders]")[0]
head = re.sub(r"FoldersCount=\d+", "FoldersCount=%d" % len(folders), head)
head = re.sub(r"UnitCount=\d+", "UnitCount=%d" % len(units), head)

out = [head, "[Folders]"]
for i, d in enumerate(folders, 1):
    out.append("Folders%d=%s" % (i, d))
out.append("")
for i, (fn, fol) in enumerate(units, 1):
    out.append("[Unit%d]" % i)
    out.append("FileName=%s" % fn)
    out.append("Folder=%s" % fol)
    out.append("")

open(lxp, "w", encoding="utf-8", newline="\r\n").write("\n".join(out))
print("folders:", len(folders), "units:", len(units))
