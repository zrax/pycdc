# Generate Python 3.14 opcode support for pycdc
# Author: ENI — patches enum pool + writes bytes/python_3_14.cpp
import dis
import re

ROOT = r"G:\wenjianqu\tools\pycdc"

# 1) Existing enum pool — exact referenceable names:
#    OPCODE(X) -> X ; OPCODE_A(X) -> X_A ; OPCODE_A_FIRST(X) -> X_A
inl_path = ROOT + r"\bytecode_ops.inl"
pool = set()
for m in re.finditer(r"(OPCODE_A_FIRST|OPCODE_A|OPCODE)\((\w+)\)", open(inl_path, encoding="utf-8").read()):
    pool.add(m.group(2) + ("_A" if m.group(1) != "OPCODE" else ""))

# 2) Authoritative 3.14 opcode list from the live interpreter
ops = [(i, n) for i, n in enumerate(dis.opname) if not n.startswith("<")]

def enum_name(i, n):
    prefer = n + ("_A" if i in dis.hasarg else "")
    fallback = n + ("" if i in dis.hasarg else "_A")
    # pycdc's enum pool is authoritative for the _A suffix convention
    if prefer not in pool and fallback in pool:
        return fallback
    return prefer

# 3) Append missing declarations to the enum pool
missing = [(i, n) for i, n in ops if enum_name(i, n) not in pool]
with open(inl_path, "a", encoding="utf-8") as f:
    f.write("\n")
    for i, n in missing:
        if i in dis.hasarg:
            line = "OPCODE_A(" + n + ")"
        else:
            line = "OPCODE(" + n + ")"
        pad = max(1, 41 - len(line))
        f.write(line + " " * pad + "// Python 3.14 ->\n")

# 4) Write the full 3.14 bytecode map
with open(ROOT + r"\bytes\python_3_14.cpp", "w", encoding="utf-8") as f:
    f.write('#include "bytecode_map.h"\n\nBEGIN_MAP(3, 14)\n')
    for i, n in ops:
        f.write("    MAP_OP(%d, %s)\n" % (i, enum_name(i, n)))
    f.write("END_MAP()\n")

print("appended %d decls | wrote %d map entries" % (len(missing), len(ops)))
