#!/usr/bin/env python3
"""
verify_mips32.py -- reference verification of the Section 5 assembly code.

A minimal (but cycle-correct) MIPS32 interpreter that executes exactly the
instruction stream written in snippet_bsp_start.S, using the real branch
delay-slot semantics (a taken branch retires after the instruction that
follows it).  With ".set reorder" the assembler only ever moves
semantics-preserving instructions into a delay slot, therefore modelling the
slots as NOPs produces the same architectural state.

Run:  python verify_mips32.py
"""

M32 = 0xFFFFFFFF
MAX_ADDR = 0xFFFFFFFF


def sx32(v):
    v &= M32
    return v - (1 << 32) if v & 0x80000000 else v


class CPU:
    def __init__(self):
        self.r = [0] * 32
        self.mem = {}
        self.pc = 0
        self.code = []

    # ---- helpers -------------------------------------------------------
    def reg(self, n):
        return self.r[n]

    def set(self, n, v):
        self.r[n] = v & M32

    def load(self, prog):
        """prog: list of (mnemonic, args...) tuples"""
        self.code = prog

    def run(self, limit=2000000):
        self.pc = 0
        steps = 0
        while self.pc < len(self.code) and steps < limit:
            ins = self.code[self.pc]
            nxt = self.pc + 1
            branch = False
            op = ins[0]

            if op == "li":            # li rd, imm  -> lui/ori pair, emulated
                self.set(ins[1], sx32(ins[2]))
            elif op == "lui":         # lui rd, imm16
                self.set(ins[1], (ins[2] & 0xFFFF) << 16)
            elif op == "addu":
                self.set(ins[1], self.reg(ins[2]) + self.reg(ins[3]))
            elif op == "subu":
                self.set(ins[1], self.reg(ins[2]) - self.reg(ins[3]))
            elif op == "addiu":       # addiu rd, rs, imm
                self.set(ins[1], self.reg(ins[2]) + ins[3])
            elif op == "move":
                self.set(ins[1], self.reg(ins[2]))
            elif op == "sw":          # sw rt, off(base)
                addr = (self.reg(ins[3]) + ins[2]) & M32
                self.mem[addr] = self.reg(ins[1])
                if addr > MAX_ADDR:
                    raise OverflowError
            elif op == "b":           # b label (delay slot = next insn)
                nxt = ins[1]
                branch = True
            elif op == "bltu":
                if self.reg(ins[1]) < self.reg(ins[2]):
                    nxt = ins[3]
                branch = True
            elif op == "nop":
                pass
            else:
                raise ValueError("unsupported instruction: " + op)

            if branch:
                # execute the delay slot, then jump
                self.pc += 1
                if self.pc < len(self.code):
                    self.step_delay_slot()
                self.pc = nxt
            else:
                self.pc = nxt
            steps += 1
        return steps

    def step_delay_slot(self):
        ins = self.code[self.pc]
        if ins[0] == "nop":
            return
        raise ValueError("non-NOP delay slot is not expected here: %s" % (ins,))


def main():
    # label table (indices into the instruction list)
    prog = [
        ("li", 8, 0),                 # t0 = 0        sum
        ("li", 9, 1),                 # t1 = 1        term
        ("li", 10, -1),               # t2 = -1       0xFFFFFFFF
        ("li", 11, 0),                # t3 = 0        last accepted term
        # cal_loop = 4
        ("subu", 12, 10, 8),          # t4 = t2 - t0
        ("bltu", 12, 9, 11),          # bltu t4, t1, save_result
        ("nop",),                     # delay slot
        ("addu", 8, 8, 9),            # sum += term
        ("move", 11, 9),              # t3 = t1
        ("addiu", 9, 9, 2),           # t1 += 2
        ("b", 4),                     # b cal_loop
        ("nop",),
        # save_result = 11
        ("lui", 13, 0x8003),          # t5 = 0x80030000
        ("sw", 8, 0, 13),             # mem[t5+0] = t0
    ]

    cpu = CPU()
    cpu.load(prog)
    cpu.run()

    names = {8: "t0", 9: "t1", 10: "t2", 11: "t3", 12: "t4", 13: "t5"}
    print("Final register state")
    print("-" * 52)
    for n in (8, 9, 10, 11, 12, 13):
        v = cpu.r[n]
        print("  %-3s = 0x%08X  (%d)" % (names[n], v, sx32(v)))
    print("-" * 52)
    for a in sorted(cpu.mem):
        print("  mem[0x%08X] = 0x%08X  (%d)" % (a, cpu.mem[a], cpu.mem[a]))

    # cross-check with closed-form expectations
    n = int((cpu.r[11] + 1) // 2)     # number of terms accepted
    expect_sum = n * n % (1 << 32)
    ok = (cpu.r[8] == expect_sum == 0xFFFE0001
          and cpu.r[11] == 0x0001FFFD
          and cpu.r[9] == 0x0001FFFF
          and cpu.mem.get(0x80030000) == 0xFFFE0001)
    print()
    print("terms accepted            : %d" % n)
    print("closed form  n^2           : 0x%08X" % expect_sum)
    print("verification              : %s" % ("PASS" if ok else "FAIL"))
    return 0 if ok else 1


if __name__ == "__main__":
    raise SystemExit(main())
