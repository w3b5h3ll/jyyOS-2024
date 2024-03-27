// Copyright 2022 Charles Lohr (MIT Licenced)
// 
// This is an "embedded" RISC-V32IMA system emulator. Original repo:
// https://github.com/cnlohr/mini-rv32ima.
//
// The take-away message from Introduction to Computer Systems:
//
//     "Everything is a state machine."
// 
// See: The RISC-V Reader (http://www.riscvbook.com/)

#include <stdint.h>

enum RV32IMA_REG {
    Z,  // x0: Zero Register.
        //     Always wired to 0 and ignores writes.
    RA, // x1: Return Address.
        //     Used by convention to store the return address in function
        //     calls. jal and jalr store the address to return.
    SP, // x2: Stack Pointer.
        //     Points to the top of the stack in memory.
    GP, // x3: Global Pointer.
        //     Typically points to the middle of the global data segment.
    TP, // x4: Thread Pointer.
        //     Used for thread-local storage.
    T0, // x5-x7: Temporary Registers.
    T1, //     Temporary storage by assembly code and compilers. Do not need
    T2, //     to be preserved across function calls.
    S0, // x8-x9: Saved Registers.
    S1, //     Store values that should be preserved across function calls.
        //     S0 is also known as FP (Frame Pointer) in some calling
        //     conventions.
    A0, // x10-x17: Argument Registers.
    A1, //     Eight arguments to functions. 
    A2, //     A0 and A1 are also used to return values from functions
    A3, A4, A5, A6, A7,
    S2, // x18-x27: More Saved Registers. 
    S3, //     Like S0 and S1, these registers are used to store values that
    S4, //     need to be preserved across function calls
    S5, S6, S7, S8, S9, S10, S11,
    T3, // x28-x31: More Temporary Registers. 
    T4, //     Like T0-T2, their values do not need to be preserved across
    T5, //     function calls
    T6,
};

enum RV32IMA_CSR {
    PC,          // Program Counter: Address of the current instruction
    MSTATUS,     // Machine Status Register: Global interrupt enable,
                 // previous privilege mode, and other status bits
    CYCLEL,      // Cycle Counter Low: Lower 32 bits of the cycle counter;
                 // counts the number of cycles since reset
    CYCLEH,      // Cycle Counter High: Upper 32 bits of the cycle counter
    TIMERL,      // Timer Low: Lower 32 bits of the real-time counter;
                 // increments at a constant rate
    TIMERH,      // Timer High: Upper 32 bits of the real-time counter
    TIMERMATCHL, // Timer Match Low: Lower 32 bits of the timer match register
                 // for setting timer interrupts
    TIMERMATCHH, // Timer Match High: Upper 32 bits of the timer match register
    MSCRATCH,    // Machine Scratch: Scratch register for trap handler
    MTVEC,       // Machine Trap-Vector Base-Address Register: Base address of
                 // the trap vector
    MIE,         // Machine Interrupt Enable: Which interrupts are enabled
    MIP,         // Machine Interrupt Pending: Which interrupts are pending
    MEPC,        // Machine Exception PC: Address to return to after exception
    MTVAL,       // Machine Trap Value: Additional information about the trap
    MCAUSE,      // Machine Cause Register: The cause of the last trap
    EXTRAFLAGS,  // Extra Flags: Processor internal decode states
                 // (not part of the standard RISC-V specification)

    CSR_COUNT,   // Number of CSRs: Utility value; the number of CSRs
                 // (Comments above are generated by GPT)
};

struct CPUState {
    // Processor internal state
    uint32_t regs[32], csrs[CSR_COUNT];

    // Memory state
    uint8_t *mem;
    uint32_t mem_offset, mem_size;
};

// 状态机，risc-v模拟器
// 操作系统，软硬件之间的中间层
static inline int32_t rv32ima_step(struct CPUState *state, uint32_t elapsedUs) {
    #define CSR(x) (state->csrs[x])
    #define REG(x) (state->regs[x])
    #define MEM(x) (&state->mem[x])

    uint32_t new_timer = CSR(TIMERL) + elapsedUs;
    if (new_timer < CSR(TIMERL)) {
        CSR(TIMERH)++;
    }

    CSR(TIMERL) = new_timer;

    // Handle Timer interrupt.

    uint64_t timer = ((uint64_t)CSR(TIMERH) << 32) | CSR(TIMERL);
    uint64_t timermatch = ((uint64_t)CSR(TIMERMATCHH) << 32) | CSR(TIMERMATCHL);

    if ((CSR(TIMERMATCHH) || CSR(TIMERMATCHL)) && (timer > timermatch)) {
        CSR(EXTRAFLAGS) &= ~4;
        CSR(MIP) |= 1 << 7;
    } else {
        CSR(MIP) &= ~(1 << 7);
    }

    // If WFI (waiting for interrupt), don't run processor.
    if (CSR(EXTRAFLAGS) & 4)
        return 1;

    uint32_t trap = 0;
    uint32_t rval = 0;
    uint32_t pc = CSR(PC);
    uint32_t cycle = CSR(CYCLEL);

    // Timer interrupt.
    if ((CSR(MIP) & (1 << 7)) && (CSR(MIE) & (1 << 7) /*mtie*/) && (CSR(MSTATUS) & 0x8 /*mie*/)) {
        trap = 0x80000007;
        pc -= 4;
        goto cycle_end;
    }

    // Otherwise, execute a single-step instruction.
    uint32_t ir = 0;
    rval = 0;
    cycle++;
    uint32_t ofs_pc = pc - state->mem_offset;

    if (ofs_pc >= state->mem_size) {
        trap = 1 + 1; // Handle access violation on instruction read.
        goto cycle_end;
    } else if (ofs_pc & 3) {
        trap = 1 + 0; // Handle PC-misaligned access
        goto cycle_end;
    } else {
        ir = *(uint32_t *)MEM(ofs_pc);
        uint32_t rdid = (ir >> 7) & 0x1f;

        switch (ir & 0x7f) {
        case 0x37: // LUI (0b0110111)
            rval = (ir & 0xfffff000); break;
        case 0x17: // AUIPC (0b0010111)
            rval = pc + (ir & 0xfffff000); break;
        case 0x6F: { // JAL (0b1101111)
            int32_t reladdy = ((ir & 0x80000000) >> 11) | ((ir & 0x7fe00000) >> 20) | ((ir & 0x00100000) >> 9) | ((ir & 0x000ff000));
            if (reladdy & 0x00100000)
                reladdy |= 0xffe00000; // Sign extension.
            rval = pc + 4;
            pc = pc + reladdy - 4;
            break;
        }
        case 0x67: { // JALR (0b1100111)
            uint32_t imm = ir >> 20;
            int32_t imm_se = imm | ((imm & 0x800) ? 0xfffff000 : 0);
            rval = pc + 4;
            pc = ((REG((ir >> 15) & 0x1f) + imm_se) & ~1) - 4;
            break;
        }
        case 0x63: { // Branch (0b1100011)
            uint32_t immm4 = ((ir & 0xf00) >> 7) | ((ir & 0x7e000000) >> 20) | ((ir & 0x80) << 4) | ((ir >> 31) << 12);
            if (immm4 & 0x1000)
                immm4 |= 0xffffe000;
            int32_t rs1 = REG((ir >> 15) & 0x1f);
            int32_t rs2 = REG((ir >> 20) & 0x1f);
            immm4 = pc + immm4 - 4;
            rdid = 0;
            switch ((ir >> 12) & 0x7)
            {
            // BEQ, BNE, BLT, BGE, BLTU, BGEU
            case 0: if (rs1 == rs2) pc = immm4; break;
            case 1: if (rs1 != rs2) pc = immm4; break;
            case 4: if (rs1 < rs2) pc = immm4; break;
            case 5: if (rs1 >= rs2) pc = immm4; break; // BGE
            case 6: if ((uint32_t)rs1 < (uint32_t)rs2) pc = immm4; break; // BLTU
            case 7: if ((uint32_t)rs1 >= (uint32_t)rs2) pc = immm4; break; // BGEU
            default:
                trap = (2 + 1);
            }
            break;
        }
        case 0x03: { // Load (0b0000011)
            uint32_t rs1 = REG((ir >> 15) & 0x1f);
            uint32_t imm = ir >> 20;
            int32_t imm_se = imm | ((imm & 0x800) ? 0xfffff000 : 0);
            uint32_t rsval = rs1 + imm_se;

            rsval -= state->mem_offset;
            if (rsval >= state->mem_size - 3) {
                rsval += state->mem_offset;
                if (rsval >= 0x10000000 && rsval < 0x12000000) {
                    if (rsval == 0x1100bffc) rval = CSR(TIMERH);
                    else if (rsval == 0x1100bff8) rval = CSR(TIMERL);
                } else {
                    trap = (5 + 1);
                    rval = rsval;
                }
            } else {
                switch ((ir >> 12) & 0x7)
                {
                // LB, LH, LW, LBU, LHU
                case 0: rval = *(int8_t *)MEM(rsval); break;
                case 1: rval = *(int16_t *)MEM(rsval); break;
                case 2: rval = *(uint32_t *)MEM(rsval); break;
                case 4: rval = *(uint8_t *)MEM(rsval); break;
                case 5: rval = *(uint16_t *)MEM(rsval); break;
                default: trap = (2 + 1);
                }
            }
            break;
        }
        case 0x23: { // Store (0b0100011)
            uint32_t rs1 = REG((ir >> 15) & 0x1f);
            uint32_t rs2 = REG((ir >> 20) & 0x1f);
            uint32_t addy = ((ir >> 7) & 0x1f) | ((ir & 0xfe000000) >> 20);
            if (addy & 0x800)
                addy |= 0xfffff000;
            addy += rs1 - state->mem_offset;
            rdid = 0;

            if (addy >= state->mem_size - 3) {
                addy += state->mem_offset;
                if (addy >= 0x10000000 && addy < 0x12000000) {
                    // Should be stuff like SYSCON, 8250, CLNT
                    if (addy == 0x11004004) // CLNT
                        CSR(TIMERMATCHH) = rs2;
                    else if (addy == 0x11004000) // CLNT
                        CSR(TIMERMATCHL) = rs2;
                    else if (addy == 0x11100000) { // SYSCON (reboot, poweroff, etc.)
                        CSR(PC) = pc + 4;
                        return rs2; // NOTE: PC will be PC of Syscon.
                    }
                }
                else
                {
                    trap = (7 + 1); // Store access fault.
                    rval = addy;
                }
            } else {
                switch ((ir >> 12) & 0x7) { // SB, SH, SW
                case 0: *(uint8_t *)MEM(addy) = rs2; break;
                case 1: *(uint16_t *)MEM(addy) = rs2; break;
                case 2: *(uint32_t *)MEM(addy) = rs2; break;
                default:
                    trap = (2 + 1);
                }
            }
            break;
        }
        case 0x13:   // Op-immediate 0b0010011
        case 0x33: { // Op           0b0110011
            uint32_t imm = ir >> 20;
            imm = imm | ((imm & 0x800) ? 0xfffff000 : 0);
            uint32_t rs1 = REG((ir >> 15) & 0x1f);
            uint32_t is_reg = !!(ir & 0x20);
            uint32_t rs2 = is_reg ? REG(imm & 0x1f) : imm;

            if (is_reg && (ir & 0x02000000)) {
                switch ((ir >> 12) & 7) { // 0x02000000 = RV32M
                case 0: rval = rs1 * rs2; break; // MUL
                case 1: rval = ((int64_t)((int32_t)rs1) * (int64_t)((int32_t)rs2)) >> 32; break; // MULH
                case 2: rval = ((int64_t)((int32_t)rs1) * (uint64_t)rs2) >> 32; break; // MULHSU
                case 3: rval = ((uint64_t)rs1 * (uint64_t)rs2) >> 32; break; // MULHU
                case 4:
                    if (rs2 == 0)
                        rval = -1;
                    else
                        rval = ((int32_t)rs1 == INT32_MIN && (int32_t)rs2 == -1) ? rs1 : ((int32_t)rs1 / (int32_t)rs2);
                    break; // DIV
                case 5:
                    if (rs2 == 0)
                        rval = 0xffffffff;
                    else
                        rval = rs1 / rs2;
                    break; // DIVU
                case 6:
                    if (rs2 == 0)
                        rval = rs1;
                    else
                        rval = ((int32_t)rs1 == INT32_MIN && (int32_t)rs2 == -1) ? 0 : ((uint32_t)((int32_t)rs1 % (int32_t)rs2));
                    break; // REM
                case 7:
                    if (rs2 == 0)
                        rval = rs1;
                    else
                        rval = rs1 % rs2;
                    break; // REMU
                }
            } else {
                switch ((ir >> 12) & 7) { // These could be either op-immediate or op commands.  Be careful.
                case 0: rval = (is_reg && (ir & 0x40000000)) ? (rs1 - rs2) : (rs1 + rs2); break;
                case 1: rval = rs1 << (rs2 & 0x1F); break;
                case 2: rval = (int32_t)rs1 < (int32_t)rs2; break;
                case 3: rval = rs1 < rs2; break;
                case 4: rval = rs1 ^ rs2; break;
                case 5: rval = (ir & 0x40000000) ? (((int32_t)rs1) >> (rs2 & 0x1F)) : (rs1 >> (rs2 & 0x1F)); break;
                case 6: rval = rs1 | rs2; break;
                case 7: rval = rs1 & rs2; break;
                }
            }
            break;
        }
        case 0x0f:    // 0b0001111
            rdid = 0; // fencetype = (ir >> 12) & 0b111; We ignore fences in this impl.
            break;
        case 0x73: { // Zifencei+Zicsr  (0b1110011)
            uint32_t csrno = ir >> 20;
            uint32_t microop = (ir >> 12) & 0x7;
            if ((microop & 3)) { // It's a Zicsr function.
                int rs1imm = (ir >> 15) & 0x1f;
                uint32_t rs1 = REG(rs1imm);
                uint32_t writeval = rs1;

                switch (csrno) {
                case 0x340: rval = CSR(MSCRATCH); break;
                case 0x305: rval = CSR(MTVEC); break;
                case 0x304: rval = CSR(MIE); break;
                case 0xC00: rval = cycle; break;
                case 0x344: rval = CSR(MIP); break;
                case 0x341: rval = CSR(MEPC); break;
                case 0x300: rval = CSR(MSTATUS); break; // mstatus
                case 0x342: rval = CSR(MCAUSE); break;
                case 0x343: rval = CSR(MTVAL); break;
                case 0xf11: rval = 0xff0ff0ff; break; // mvendorid
                case 0x301: rval = 0x40401101; break; // misa (XLEN=32, IMA+X)
                default:
                    break;
                }

                switch (microop) {
                case 1: writeval = rs1; break; // CSRRW
                case 2: writeval = rval | rs1; break; // CSRRS
                case 3: writeval = rval & ~rs1; break; // CSRRC
                case 5: writeval = rs1imm; break; // CSRRWI
                case 6: writeval = rval | rs1imm; break; // CSRRSI
                case 7: writeval = rval & ~rs1imm; break; // CSRRCI
                }

                switch (csrno) {
                case 0x340: CSR(MSCRATCH) = writeval; break;
                case 0x305: CSR(MTVEC) = writeval; break;
                case 0x304: CSR(MIE) = writeval; break;
                case 0x344: CSR(MIP) = writeval; break;
                case 0x341: CSR(MEPC) = writeval; break;
                case 0x300: CSR(MSTATUS) = writeval; break; // mstatus
                case 0x342: CSR(MCAUSE) = writeval; break;
                case 0x343: CSR(MTVAL) = writeval; break;
                default:
                    break;
                }
            }
            else if (microop == 0x0) { // "SYSTEM" 0b000
                rdid = 0;
                if (csrno == 0x105) { // WFI (Wait for interrupts)
                    CSR(MSTATUS) |= 8;    // Enable interrupts
                    CSR(EXTRAFLAGS) |= 4; // Infor environment we want to go to sleep.
                    CSR(PC) = pc + 4;
                    return 1;
                } else if (((csrno & 0xff) == 0x02)) { // MRET
                    uint32_t startmstatus = CSR(MSTATUS);
                    uint32_t startextraflags = CSR(EXTRAFLAGS);
                    CSR(MSTATUS) = ((startmstatus & 0x80) >> 4) | ((startextraflags & 3) << 11) | 0x80;
                    CSR(EXTRAFLAGS) = (startextraflags & ~3) | ((startmstatus >> 11) & 3);
                    pc = CSR(MEPC) - 4;
                } else {
                    switch (csrno) {
                    case 0:
                        trap = (CSR(EXTRAFLAGS) & 3) ? (11 + 1) : (8 + 1);
                        break; // ECALL; 8 = "Environment call from U-mode"; 11 = "Environment call from M-mode"
                    case 1:
                        trap = (3 + 1);
                        break; // EBREAK 3 = "Breakpoint"
                    default:
                        trap = (2 + 1);
                        break; // Illegal opcode.
                    }
                }
            }
            else
                trap = (2 + 1);
            break;
        }
        case 0x2f: { // RV32A (0b00101111)
            uint32_t rs1 = REG((ir >> 15) & 0x1f);
            uint32_t rs2 = REG((ir >> 20) & 0x1f);
            uint32_t irmid = (ir >> 27) & 0x1f;

            rs1 -= state->mem_offset;

            // We don't implement load/store from UART or CLNT with RV32A here.

            if (rs1 >= state->mem_size - 3) {
                trap = (7 + 1); // Store/AMO access fault
                rval = rs1 + state->mem_offset;
            } else {
                rval = *(uint32_t *)MEM(rs1);

                // Referenced a little bit of https://github.com/franzflasch/riscv_em/blob/master/src/core/core.c
                uint32_t dowrite = 1;
                switch (irmid) {
                case 2: // LR.W (0b00010)
                    dowrite = 0;
                    CSR(EXTRAFLAGS) = (CSR(EXTRAFLAGS) & 0x07) | (rs1 << 3);
                    break;
                case 3:                                                  // SC.W (0b00011) (Make sure we have a slot, and, it's valid)
                    rval = (CSR(EXTRAFLAGS) >> 3 != (rs1 & 0x1fffffff)); // Validate that our reservation slot is OK.
                    dowrite = !rval;                                     // Only write if slot is valid.
                    break;
                case 1:
                    break; // AMOSWAP.W (0b00001)
                case 0: rs2 += rval; break; // AMOADD.W (0b00000)
                case 4: rs2 ^= rval; break; // AMOXOR.W (0b00100)
                case 12: rs2 &= rval; break; // AMOAND.W (0b01100)
                case 8: rs2 |= rval; break; // AMOOR.W (0b01000)
                case 16: rs2 = ((int32_t)rs2 < (int32_t)rval) ? rs2 : rval; break; // AMOMIN.W (0b10000)
                case 20: rs2 = ((int32_t)rs2 > (int32_t)rval) ? rs2 : rval; break; // AMOMAX.W (0b10100)
                case 24: rs2 = (rs2 < rval) ? rs2 : rval; break; // AMOMINU.W (0b11000)
                case 28: rs2 = (rs2 > rval) ? rs2 : rval; break; // AMOMAXU.W (0b11100)
                default:
                    trap = (2 + 1);
                    dowrite = 0;
                    break; // Not supported.
                }
                if (dowrite)
                    *(uint32_t *)MEM(rs1) = rs2;
            }
            break;
        }
        default:
            trap = (2 + 1); // Fault: Invalid opcode.
        }

        // If there was a trap, do NOT allow register writeback.
        if (trap)
            goto cycle_end;

        if (rdid) {
            state->regs[rdid] = rval;
        }
    }

    pc += 4;

cycle_end:
    // Handle traps and interrupts.
    if (trap) {
        if (trap & 0x80000000) { // It's an interrupt, not a trap.
            CSR(MCAUSE) = trap;
            CSR(MTVAL) = 0;
            pc += 4; // PC needs to point to where the PC will return to.
        } else {
            CSR(MCAUSE) = trap - 1;
            CSR(MTVAL) = (trap > 5 && trap <= 8) ? rval : pc;
        }
        CSR(MEPC) = pc;
        // On an interrupt, the system moves current MIE into MPIE
        CSR(MSTATUS) = ((CSR(MSTATUS) & 0x08) << 4) | ((CSR(EXTRAFLAGS) & 3) << 11);
        pc = (CSR(MTVEC) - 4);

        // If trapping, always enter machine mode.
        CSR(EXTRAFLAGS) |= 3;

        trap = 0;
        pc += 4;
    }

    if (CSR(CYCLEL) > cycle)
        CSR(CYCLEH)++;
    CSR(CYCLEL) = cycle;
    CSR(PC) = pc;
    return 0;
}
