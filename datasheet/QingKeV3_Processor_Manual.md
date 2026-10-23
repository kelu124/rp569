QingKeV3 Microprocessor Manual
V1.5
Overview
The QingKe V3 series microprocessors are 32-bit general-purpose MCU microprocessors developed based on
the standard RISC-V instruction set architecture. This series includes five models: V3A, V3B, V3C, V3F, and
V3V.
 V3A: Support RV32IMAC standard instruction set extensions.
 V3B: Support RV32I[M]C[B] standard instruction sets and XW custom instruction set extensions;
some chips do not support 'M' or 'B' instructions.
 V3C: Support the RV32IMCB standard instruction set and XW custom instruction set extensions.
 V3F: Support the RV32IMAFCB standard instruction set and XW custom instruction set extensions.
 V3V: Support the RV32IMACB_Zve64x_zvbb standard instruction set and XW custom instruction set
extensions.
Additionally, this series of microprocessors incorporates several distinctive features, including Hardware
Prologue/Epilogue (HPE), Vector Table Free (VTF), streamlined 1-/2-wire debugging interface, and “WFI”
and “WFE” sleep modes.
Features
Features Description
ISA RV32I[M][A][F]C[B][V][XW]
Pipeline 3
FPU V3F supports scalar floating-point operations
Branch prediction Static branch prediction
Support a total of 256 interrupts including exceptions, and
Interrupt
supports VTF
Automatic stack protection Support up to level 2 automatic stack protection
Physical Memory Protection
Supported
(PMP)
Support Sleep and Deep sleep modes, and support WFI and WFE
Low-power consumption mode
sleep methods
Extended Instruction Set Supported
Debug 1/2-wire SDI, standard RISC-V debug

|     | QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |     |
| --- | ------------------------------- | --- | --- | --- | --- | --- | ------------------- | --- |
Chapter 1 Overview
QingKe V3 series microprocessors include V3A, V3B, V3C, V3F, V3V, there are some differences between
the series according to the application, the specific differences are detailed in Table 1-1.
Table 1-1 Overview of QingKe V3 microprocessor
Interruptions
|        | Feature  |      | HPE        |          | VTF                 |             | Extended     | Number of         |
| ------ | -------- | ---- | ---------- | -------- | ------------------- | ----------- | ------------ | ----------------- |
|        |          |      |            | nesting  |                     | Vector      |              |                   |
|        |          | ISA  | number of  |          | number of Pipeline  |             | Instruction  | memory            |
|        |          |      | number of  |          |                     | table mode  |              |                   |
| Model  |          |      | levels     |          | channels            |             | (XW)         | protection areas  |
levels
|     | V3A  | RV32IMAC  | 2   | 2   | 4  3  | Instruction  | ×   | ×   |
| --- | ---- | --------- | --- | --- | ----- | ------------ | --- | --- |
Address/
|     | V3B  RV32I[M]C[B]  |     | 2   | 2   | 4  3  |     | √   | ×   |
| --- | ------------------ | --- | --- | --- | ----- | --- | --- | --- |
Instruction
Address/
|     | V3C  | RV32IMCB  | 2   | 2   | 4  3  |     | √   | 4   |
| --- | ---- | --------- | --- | --- | ----- | --- | --- | --- |
Instruction
Address/
|     | V3F  RV32IMAFCB  |     | 2   | 2   | 4  3  |     | √   | 4   |
| --- | ---------------- | --- | --- | --- | ----- | --- | --- | --- |
Instruction
Address/
|     | V3V  RV32IMACBV  |     | 2   | 2   | 4  3  |     | √   | 4   |
| --- | ---------------- | --- | --- | --- | ----- | --- | --- | --- |
Instruction
Note: OS task switching generally uses stack push, which are not limited in number of levels

1.1 Instruction Set
QingKe V3 series microprocessors follow the standard RISC-V Instruction Set Architecture (ISA). Detailed
documentation of the standard can be found in 'The RISC-V Instruction Set Manual, Volume I: User-Level
ISA, Document Version 2.2' on the RISC-V International website. The RISC-V instruction set has a simple
architecture and supports a modular design, allowing for flexible combinations based on different needs, and
the V3 series supports the following instruction set extensions.
|     |   RV32: 32-bit architecture, general-purpose register bit width of 32 bits  |     |     |     |     |     |     |     |
| --- | ---------------------------------------------------------------------------- | --- | --- | --- | --- | --- | --- | --- |
|     |   I: Support integer operation, with 32 integer registers                   |     |     |     |     |     |     |     |
|     |   M: Support integer multiplication and division instructions               |     |     |     |     |     |     |     |
  A: Support atomic instructions, in which V3A simplifies the lr and sc instructions and executes them only
as lw, sw, and the sc result return is always successful
|     |   C: Support 16-bit compression instruction                |     |     |     |     |     |     |     |
| --- | ----------------------------------------------------------- | --- | --- | --- | --- | --- | --- | --- |
|     |   B: Support for bit manipulation instructions             |     |     |     |     |     |     |     |
|     |   F: Support single-precision floating-point instructions  |     |     |     |     |     |     |     |
  V: Support the zve64x and avbb instruction sets, with a 64-bit vector register width.
  XW: 16-bit compression instructions for self-extending byte and halfword operations
Note:
1: The subset of instructions supported by different models may be different, please refer to Table 1-1 for
details;
2: In order to further improve the code density, extend the XW subset, add the following compression
instructions c.lbu/c.lhu/c.sb/c.sh/c.lbusp/c.lhusp/c.sbsp/c.shsp, use of which needs to be based on the MRS
compiler or the toolchain it provides;
3: V3B/C supports memory copy instruction. For specific usage, please refer to library function and cooperate
with MRS compiler or its tool chain.
|     | V1.5  |     |     |     | 1   |     |     |     |
| --- | ----- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | ------------------- |

1.2 Register Set
The RV32I has 32 register sets from x0-x31. In the RV32, each register is 32 bits. Table 1-2 below lists the
registers of RV32I and their descriptions.
Table 1-2 RV32I general-purpose registers
| Register  | ABI Name  | Description                        | Storer  |
| --------- | --------- | ---------------------------------- | ------- |
| x0        | zero      | Hardcoded 0                        | -       |
| x1        | ra        | Return address                     | Caller  |
| x2        | sp        | Stack pointer                      | Callee  |
| x3        | gp        | Global pointer                     | -       |
| x4        | tp        | Thread pointer                     | -       |
| x5-7      | t0-2      | Temporary register                 | Caller  |
| x8        | s0/fp     | Save register/frame pointer        | Callee  |
| x9        | s1        | Save register                      | Callee  |
| x10-11    | a0-1      | Function parameters/return values  | Caller  |
| x12-17    | a2-7      | Function parameters                | Caller  |
| x18-27    | a2-11     | Save register                      | Callee  |
| X28-31    | t3-6      | Temporary register                 | Caller  |
The Caller attribute in the above table means that the called procedure does not save the register value, and
the Callee attribute means that the called procedure saves the register.

The V3F model kernel supports the F instruction set and has a total of 32 floating-point registers, F0-F31,
which are used to store data required for single-precision floating-point instructions. Table 1-3 below lists the
floating-point registers and their descriptions.
Table 1-3 RISC-V3F registers
| Register  | ABI Name  | Description                        | Storer  |
| --------- | --------- | ---------------------------------- | ------- |
| f0-7      | ft0-7     | Floating point temporary register  | Caller  |
| f8-9      | fs0-1     | Floating point save register       | Callee  |
Floating point function parameter
| f10-11  | fa0-1  |     | Caller  |
| ------- | ------ | --- | ------- |
/return values
| f12-17  | fa2-7   | Floating point function parameter  | Caller  |
| ------- | ------- | ---------------------------------- | ------- |
| f18-27  | fs2-11  | Floating point save register       | Callee  |
| f28-31  | ft8-11  | Floating point temporary register  | Caller  |
The Caller attribute in the above table means that the called process does not save the value of this register,
and the Callee attribute means that the called process saves this register.

The V3V model kernel supports the V instruction set, featuring 32 vector register groups from V0 to V31.
Each vector register has a 64-bit width and supports vector operations with 64-, 32-, 16-, and 8-bit widths.
Table 1-4 RISC-V3V registers
| Register  | ABI Name  | Description      | Storer  |
| --------- | --------- | ---------------- | ------- |
| V0-V31    | v0-v31    | Vector register  | Callee  |
Note: Vector registers support both standard ABI and variant ABI storage formats during invocation. For
detailed ABI invocation specifications, refer to the RISC-V ABIs Specification v1.1 and the CSR.vcontrol
register configuration.

1.3 Privilege Mode
The standard RISC-V architecture includes 3 privileged modes: Machine mode, Supervisor mode, and User
mode, as shown in Table 1-5 below. The machine mode is a mandatory mode, and the other modes are optional
V1.5  2

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
modes. For details, you can refer to "The RISC-V Instruction Set Manual Volume II: Privileged Architecture",
which can be downloaded for free from the RISC-V International website.
Table 1-5 RISC-V architecture privilege mode
|     | Code  | Name              | Abbreviations  |     |
| --- | ----- | ----------------- | -------------- | --- |
|     | 0b00  | User Mode         | U              |     |
|     | 0b01  | Supervisor Model  | S              |     |
|     | 0b10  | Reserved          | Reserved       |     |
|     | 0b11  | Machine mode      | M              |     |
QingKe V3 series microprocessors support 2 of these privileged modes.
  Machine mode
Machine mode has the highest authority, the program in this mode can access all the Control and Status
Register (CSR), but also can access all the physical address area. The power-up default is in machine mode,
when the execution of mret (Machine mode return instruction) returns, according to the CSR register mstatus
(Machine mode status register) in the MPP bit, if MPP = 0b00, then exit the Machine mode into the User mode,
MPP = 0b11, then continue to retain the Machine mode.
  User mode
User mode has the lowest privilege, and only limited CSR registers can be accessed in this mode. When an
exception or interrupt occurs, the microprocessor goes from User mode to Machine mode to handle exceptions
and interrupts.

1.4 CSR Register
A series of CSR registers are defined in the RISC-V architecture to control and record the operating state of
the microprocessor. These CSRs can be extended by 4096 registers using an internal dedicated 12-bit address
coding space. And use the high two CSR[11:10] to define the read/write permission of this register, 0b00, 0b01,
0b10 for read/write allowed and 0b11 for read only. Use the two bits CSR[9:8] to define the lowest privilege
level that can access this register, and the value corresponds to the privilege mode defined in Table 1-5. The
CSR registers implemented in the QingKe V3 microprocessor are detailed in Chapter 8.
|     |     |     |     |     |
| --- | --- | --- | --- | --- |
V1.5  3

| QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | ------------------- |
Chapter 2 Exception
Exception mechanism, which is a mechanism to intercept and handle "unusual operation events". QingKe V3
series microprocessors are equipped with an exception response system that can handle up to 256 exceptions,
including interrupts. When an exception or interruption occurs, the microprocessor can quickly respond and
handle the exception and interruption events.

2.1 Exception Types
The hardware behavior of the microprocessor is the same whether an exception or an interrupt occurs. The
microprocessor suspends the current program, moves to the exception or interrupt handler, and returns to the
previously suspended program when processing is complete. Broadly speaking, interrupts are also part of
exceptions. Whether exactly the current occurrence is an interrupt or an exception can be viewed through the
Machine mode exception cause register mcause. The mcause[31] is the interrupt field, which is used to indicate
whether  the  cause  of  the  exception  is  an  interrupt  or  an  exception.  mcause[31]=1  means  interrupt,
mcause[31]=0 means exception. mcause[30:0] is the exception code, which is used to indicate the specific
cause of the exception or the interrupt number, as shown in the following table.
Table 2-1 V3 microprocessor exception codes
|            | Exception  | Synchronous /         |     |                                   |                            |     |
| ---------- | ---------- | --------------------- | --- | --------------------------------- | -------------------------- | --- |
| Interrupt  |            |                       |     |                                   | Reason for exception       |     |
|            | codes      | Asynchronous          |     |                                   |                            |     |
| 1          | 0-1        |                       | -   |                                   | Reserved                   |     |
| 1          | 2          | Precise asynchronous  |     |                                   | NMI interrupts             |     |
| 1          | 3-11       |                       | -   |                                   | Reserved                   |     |
| 1          | 12         | Precise asynchronous  |     |                                   | SysTick interrupts         |     |
| 1          | 13         |                       | -   |                                   | Reserved                   |     |
| 1          | 14         | Synchronous           |     |                                   | Software interrupts        |     |
| 1          | 15         |                       | -   |                                   | Reserved                   |     |
| 1          | 16-255     | Precise asynchronous  |     |                                   | External interrupt 16-255  |     |
| 0          | 0          | Synchronous           |     | Instruction address misalignment  |                            |     |
| 0          | 1          | Synchronous           |     | Fetch command access error        |                            |     |
| 0          | 2          | Synchronous           |     |                                   | Illegal instructions       |     |
| 0          | 3          | Synchronous           |     |                                   | Breakpoints                |     |
Load instruction access address
| 0   | 4   | Synchronous  |     |     |     |     |
| --- | --- | ------------ | --- | --- | --- | --- |
misalignment
Non- precision
| 0   | 5   |     |     |     | Load command access error  |     |
| --- | --- | --- | --- | --- | -------------------------- | --- |
asynchronous
Store/AMO instruction access address
| 0   | 6   | Synchronous  |     |     |     |     |
| --- | --- | ------------ | --- | --- | --- | --- |
misalignment
Non-precision
| 0   | 7   |     |     | Store/AMO command access error  |     |     |
| --- | --- | --- | --- | ------------------------------- | --- | --- |
asynchronous
| 0   | 8   | Synchronous  |     | Environment call in User mode     |     |     |
| --- | --- | ------------ | --- | --------------------------------- | --- | --- |
| 0   | 11  | Synchronous  |     | Environment call in Machine mode  |     |     |

'Synchronous' in the table means that an instruction can be located exactly where it is executed, such as an
ebreak or ecall instruction, and each execution of that instruction will trigger an exception. 'Asynchronous'
means that it is not possible to pinpoint an instruction, and the instruction PC value may be different each time
an exception occurs. 'Precise asynchronous' means that after an exception occurs, it can precisely locate the
boundary of an instruction, save all core state values at the time of the current exception, and resume the
original instruction to continue executing the program after the exception is handled, such as an external
interrupt. The 'non-precise asynchronous' refers to the inability to accurately locate the boundaries of a
| V1.5  |     |     | 4   |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

QingKeV3 Microprocessor Manual https://wch-ic.com
particular instruction, may be an instruction is interrupted halfway through the execution of the state, the
completion of the exception cannot be restored to the original normal operation of the program, such as
memory access error. Access to memory takes time, the microprocessor access to memory is generally not
always waiting for the end of the access, but to continue to execute the instruction, and then the access error
exception, the microprocessor has already executed the subsequent instructions, cannot accurately locate or
save the complete abnormal instruction before the register value.
2.2 Entering Exception
When the program is in the process of normal operation, if for some reason, triggered into an exception or
interrupt. The hardware behavior of the microprocessor at this point can be summarized as follows.
(1) Suspend the current program flow and move to the execution of exception or interrupt handling functions.
The entry base address and addressing mode of the exception or interrupt function are defined by the exception
entry base address register mtvec. mtvec[31:2] defines the base address of the exception or interrupt function.
mtvec[1:0] defines the addressing mode of the handler function. when mtvec[0]=0, all exceptions and
interrupts use a unified entry, i.e., when an exception or interrupt occurs, it turns to the mtvec[31:2] defines
the base address to execute. When mtvec[0]=1, exceptions and interrupts use vector table mode, i.e., each
exception and interrupt is numbered, and the address is offset according to interrupt number*4, and when an
exception or interrupt occurs, it is shifted to the base address defined by mtvec[31:2] + interrupt number*4
Execution. The interrupt vector table holds an instruction to jump to the interrupt handler function, or it can
be other instructions.
mtvec[1] defines whether an instruction or an address is stored in the entry address. When mtvec[1]=0, the
exception entry address stores a jump instruction; when mtvec[1]=1, the exception entry address stores a jump
address.
(2) Update CSR register
When an exception or interrupt is entered, the microprocessor automatically updates the relevant CSR registers,
including the Machine mode exception cause register mcause, the Machine mode exception pointer register
mepc, the Machine mode exception value register mtval, and the Machine mode status register mstatus.
 Update mcause
As mentioned before, after entering an exception or interrupt, its value reflects the current exception type or
interrupt number, and the software can read this register value to check the cause of the exception or determine
the source of the interrupt, as detailed in Table 2-1.
 Update mepc
The standard definition of the return address of the microprocessor after exiting an exception or interrupt is
stored in mepc. So when an exception or interrupt occurs, the hardware automatically updates the mepc value
to the current instruction PC value when the exception is encountered, or the next pre-executed instruction PC
value before the interrupt. After the exception or interrupt is processed, the microprocessor uses its saved value
as the return address to return to the location of the interrupt to continue execution.
However, it is worth noting that.
1. mepc is a readable and writable register, and the software can also modify the value for the purpose of
modifying the location of the PC pointer running after the return.
2. When an interrupt occurs, i.e., when the exception cause register mcause[31]=1, the value of mepc is
V1.5 5

QingKeV3 Microprocessor Manual https://wch-ic.com
updated to the PC value of the next unexecuted instruction at the time of the interrupt.
And when an exception occurs, the value of mepc is updated to the instruction PC value of the current
exception when the exception cause register mcause[31]=0. Therefore, when the exception returns, if you use
the value of mepc to return directly, you will try to execute the exception instruction again, and then it is
possible to enter the exception repeatedly. For some exceptions, the value of mepc can be changed to the value
of the next unexecuted instruction before returning. For example, if we cause an exception due to ecall/ebreak,
after handling the exception, since ecall/ebreak (c.ebreak is 2 bytes) is a 4-byte instruction, we only need the
software to modify the value of mepc to mepc+4 (c.ebreak is mepc+2) and then return.
 Update mtval
When exceptions and interrupts are entered, the hardware will automatically update the value of mtval, which
is the value that caused the exception. The value is typically.
1. If an exception is caused by a memory access, the hardware will store the address of the memory access
at the time of the exception into mtval.
2. If the exception is caused by an illegal instruction, the hardware will store the instruction code of the
instruction into mtval.
3. If the exception is caused by a hardware breakpoint, the hardware will store the PC value at the breakpoint
into mtval.
4. For other exceptions, the hardware sets the value of mtval to 0, such as ebreak, the exception caused by
ecall instruction.
5. When entering the interrupt, the hardware sets the value of mtval to 0.
 Update mstatus
Upon entering exceptions and interrupts, the hardware updates certain bits in mstatus.
1. MPIE is updated to the MIE value before entering the exception or interrupt, and MPIE is used to restore
the MIE after the exception and interrupt are over.
2. MPP is updated to the privileged mode before entering exceptions and interrupts, and after the exceptions
and interrupts are over, MPP is used to restore the previous privileged mode.
3. QingKe V3 microprocessor supports interrupt nesting in Machine mode. After entering exceptions and
interrupts, the MIE is updated to a new value based on information such as the current nesting depth and
nesting enable.
(3) Update microprocessor privilege mode
When exceptions and interrupts occur, the privileged mode of the microprocessor is updated to Machine mode.
2.3 Exception Handling Functions
Upon entering an exception or interrupt, the microprocessor executes the program from the address and mode
defined by the mtvec register. When using unified entry, the microprocessor enters the interrupt from the base
address defined by mtvec[31:2]. At this time, the exception and interrupt handling function can determine
whether the cause is an exception or interrupt based on the value of mcause[31], and the type and cause of the
exception or the corresponding interrupt can be judged by the exception code and handled accordingly.
When using the base address + interrupt number *4 for offset, the hardware automatically jumps to the vector
table to get the entry address of the exception or interrupt function based on the interrupt number.
V1.5 6

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
2.4 Exception Exit
After the exception or interrupt handler is completed, it is necessary to exit from the service program. After
entering exceptions and interrupts, the microprocessor enters Machine mode from User mode, and the
processing of exceptions and interrupts is also completed in Machine mode. When it is necessary to exit
exceptions and interrupts, it is necessary to use the mret instruction to return. At this time, the microprocessor
hardware will automatically perform the following operations.
  The PC pointer is restored to the value of CSR register mepc, i.e., execution starts at the instruction
address saved by mepc. It is necessary to pay attention to the offset operation of mepc after the exception
handling is completed.
  Update CSR register mstatus, MIE is restored to MPIE, and MPP is used to restore the privileged mode
of the previous microprocessor.
The entire exception response process can be described by the following Figure 2-1.
Figure 2-1 Exception response process diagram
|     | Exception_Handler | “mret” | PC=mepc，Then running |     |
| --- | ----------------- | ------ | -------------------- | --- |
mepc
mcause
| Main Exception  | Update CSR |     | Update CSR | mstatus |
| --------------- | ---------- | --- | ---------- | ------- |
mtval
mstatus
 Machine Mode(Update Privileged Mode) update privileged mode(mode = MPP)

|       |     |     |     |     |
| ----- | --- | --- | --- | --- |
| V1.5  |     | 7   |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
Chapter 3 PFIC and Interrupt Control
QingKe V3 microprocessor is designed with a Programmable Fast Interrupt Controller (PFIC) that can manage
up to 256 interrupts including exceptions.
The first 16 of V3A, V3B, V3C, V3V are fixed as internal interrupts of the microprocessor, and the rest are
external interrupts, i.e. the maximum number of external interrupts can be extended to 240. Its main features
are as follows.
  240 external interrupts, each interrupt request has independent trigger and mask control bits, with
dedicated status bits
  Programmable multi-level interrupt nesting, maximum nesting depth 2 levels
  Vector Table Free (VTF) interrupt response mechanism, 4-channel programmable direct access to
interrupt vector addresses

Among them, the first 32 of V3F are fixed as microprocessor internal interrupts, and the rest are external
interrupts, i.e., it supports the expansion of a maximum of 224 external interrupts. Its main features are as
follows:
  244 external interrupts, each interrupt request has independent trigger and mask control bits, with
dedicated status bits
  Programmable multi-level interrupt nesting, maximum nesting depth 2 levels
  Special fast interrupt in/out mechanism, hardware automatic stacking and recovery, maximum HPE depth
of 2 levels
  Vector Table Free (VTF) interrupt response mechanism, 4-channel programmable direct access to
interrupt vector addresses
Note: The maximum nesting depth and HPE depth supported by interrupt controllers vary for different
microprocessor models, which can be found in Table 1-1.

The vector table of V3A, V3B, V3C, V3V interrupts and exceptions is shown in Table 3-1 below.
Table 3-1 V3A, V3B, V3C, V3V Exception and interrupt vector table
| Number  | Priority  | Type          |             | Name     |                                  | Description             |     |     |
| ------- | --------- | ------------- | ----------- | -------- | -------------------------------- | ----------------------- | --- | --- |
| 0       | -         | -             |             | -        |                                  |                         | -   |     |
| 1       | -         | -             |             | -        |                                  |                         | -   |     |
| 2       | -5        | Fixed         |             | NMI      |                                  | Non-maskable interrupt  |     |     |
| 3       | -4        | Fixed         |             | EXC      |                                  | Exception interrupt     |     |     |
| 4       | -         | -             |             | -        |                                  |                         | -   |     |
| 5       | -3        | Fixed         |             | ECALL-M  | Machine mode callback interrupt  |                         |     |     |
| 6-7     | -         | -             |             | -        |                                  |                         | -   |     |
| 8       | -2        | Fixed         |             | ECALL-U  | User mode callback interrupt     |                         |     |     |
| 9       | -1        | Fixed         | BREAKPOINT  |          | Breakpoint callback interrupt    |                         |     |     |
| 10-11   | -         | -             |             | -        |                                  |                         | -   |     |
| 12      | 0         | Programmable  |             | SysTick  |                                  | System timer interrupt  |     |     |
| 13      | -         | -             |             | -        |                                  |                         | -   |     |
| 14      | 1         | Programmable  |             | SW       |                                  | Software interrupt      |     |     |
| 15      | -         | -             |             | -        |                                  |                         | -   |     |
16-255  2-241  Programmable  External Interrupt  External interrupt 16-255
Note: ECALL-M, ECALL-U, BREAKPOINT are all different types of exception EXC, which are independent
| V1.5  |     |     |     | 8   |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
in V3B/C for ease of use, and the above 3 entry addresses are shared with EXC in V3A.

The vector table of V3F interrupts and exceptions is shown in Table 3-2 below.
Table 3-2 V3F Exception and interrupt vector table
|     | Number  | Priority  |               | Type   |             | Name       |     |                                  | Description          |     |
| --- | ------- | --------- | ------------- | ------ | ----------- | ---------- | --- | -------------------------------- | -------------------- | --- |
|     | 0       | -6        |               | Fixed  |             | RESET      |     |                                  | Reset interrupt      |     |
|     | 1       | -         |               | -      |             | -          |     |                                  | -                    |     |
|     | 2       | -5        |               | Fixed  |             | NMI        |     | Non-maskable interrupt           |                      |     |
|     | 3       | -4        |               | Fixed  |             | HardFault  |     |                                  | Exception interrupt  |     |
|     | 4       | -         |               | -      |             | -          |     |                                  | -                    |     |
|     | 5       | -3        |               | Fixed  |             | Ecall-M    |     | Machine mode callback interrupt  |                      |     |
|     | 6-7     | -         |               | -      |             | -          |     |                                  | -                    |     |
|     | 8       | -2        |               | Fixed  |             | Ecall-U    |     | User mode callback interrupt     |                      |     |
|     | 9       | -1        |               | Fixed  | BreakPoint  |            |     | Breakpoint callback interrupt    |                      |     |
|     | 10-11   | -         |               | -      |             | -          |     |                                  | -                    |     |
|     | 12      | 0         | Programmable  |        |             | SysTick0   |     | System timer interrupt 0         |                      |     |
|     | 13      | 1         | Programmable  |        |             | SysTick1   |     | System timer interrupt 1         |                      |     |
|     | 14      | 2         | Programmable  |        |             | SW         |     |                                  | Software interrupt   |     |
|     | 15      | -         |               | -      |             | -          |     |                                  | -                    |     |
16  3  Programmable  IPC_CH0  Inter-core communication interrupt 0
17  4  Programmable  IPC_CH1  Inter-core communication interrupt 1
18  5  Programmable  IPC_CH2  Inter-core communication interrupt 2
19  6  Programmable  IPC_CH3  Inter-core communication interrupt 3
|     | 20-27  | -   |               | -   |     | -     |     |                               | -   |     |
| --- | ------ | --- | ------------- | --- | --- | ----- | --- | ----------------------------- | --- | --- |
|     | 28     | 7   | Programmable  |     |     | HSEM  |     | Hardware semaphore interrupt  |     |     |
|     | 29-31  | -   |               | -   |     | -     |     |                               | -   |     |
External
|     | 32-255  | 8-224  | Programmable  |     |     |     |     | External interrupt 32-255  |     |     |
| --- | ------- | ------ | ------------- | --- | --- | --- | --- | -------------------------- | --- | --- |
Interrupt

3.1 PFIC Register Set
Table 3-3 PFIC Registers
|                | Name  |     | Access address  |     | Access  |                                   | Description  |           |            | Reset value  |
| -------------- | ----- | --- | --------------- | --- | ------- | --------------------------------- | ------------ | --------- | ---------- | ------------ |
|                |       |     | 0xE000E000      |     |         | Interrupt enable status register  |              |           |            |              |
| PFIC_ISRx      |       |     |                 |     |         | RO                                |              |           |            | 0x00000000   |
|                |       |     | -0xE000E01C     |     |         | x                                 |              |           |            |              |
|                |       |     | 0xE000E020      |     |         | Interrupt                         |              | pending   | status     |              |
| PFIC_IPRx      |       |     |                 |     |         | RO                                |              |           |            | 0x00000000   |
|                |       |     | -0xE000E03C     |     |         | register x                        |              |           |            |              |
|                |       |     |                 |     |         | Interrupt                         |              | priority  | threshold  |              |
| PFIC_ITHRESDR  |       |     | 0xE000E040      |     |         | RW                                |              |           |            | 0x00000000   |
configuration register
VTF base address register
| PFIC_VTFBADDRR  |     |     | 0xE000E044  |     |     | RW  |     |     |     | 0x00000000  |
| --------------- | --- | --- | ----------- | --- | --- | --- | --- | --- | --- | ----------- |
Note: Valid only for V3A
|            |     |     |             |     |     | Interrupt     |     |     | configuration  |             |
| ---------- | --- | --- | ----------- | --- | --- | ------------- | --- | --- | -------------- | ----------- |
| PFIC_CFGR  |     |     | 0xE000E048  |     |     | RW  register  |     |     |                | 0x00000000  |
Note: Valid only for V3A/F/V
| V1.5  |     |     |     |     |     | 9   |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
PFIC_GISR  0xE000E04C  RO  Interrupt global status register  0x00000002
|     |     |     |     | VTF  |     | interrupt  | ID  |     |
| --- | --- | --- | --- | ---- | --- | ---------- | --- | --- |
configuration register
| PFIC_VTFIDR  |     | 0xE000E050  | RW  |        |        |     |            | 0x00000000  |
| ------------ | --- | ----------- | --- | ------ | ------ | --- | ---------- | ----------- |
|              |     |             |     | Note:  | Valid  |     | only  for  |             |
V3B/C/F/V.
0xE000E060
PFIC_VTFADDRRx  RW  VTF x offset address register  0xXXXXXXXX
-0xE000E06C
|               |     | 0xE000E100   |     | Interrupt                        |             | enable   | setting   |             |
| ------------- | --- | ------------ | --- | -------------------------------- | ----------- | -------- | --------- | ----------- |
| PFIC_IENRx    |     |              | WO  |                                  |             |          |           | 0x00000000  |
|               |     | -0xE000E11C  |     | register x                       |             |          |           |             |
|               |     | 0xE000E180   |     | Interrupt enable clear register  |             |          |           |             |
| PFIC_IRERx    |     |              | WO  |                                  |             |          |           | 0x00000000  |
|               |     | -0xE000E19C  |     | x                                |             |          |           |             |
|               |     | 0xE000E200   |     | Interrupt                        |             | pending  | setting   |             |
| PFIC_IPSRx    |     |              | WO  |                                  |             |          |           | 0x00000000  |
|               |     | -0xE000E21C  |     | register x                       |             |          |           |             |
|               |     | 0xE000E280   |     | Interrupt                        |             | pending  | clear     |             |
| PFIC_IPRRx    |     |              | WO  |                                  |             |          |           | 0x00000000  |
|               |     | -0xE000E29C  |     | register x                       |             |          |           |             |
|               |     | 0xE000E300   |     | Interrupt                        | activation  |          | status    |             |
| PFIC_IACTRx   |     |              | RO  |                                  |             |          |           | 0x00000000  |
|               |     | -0xE000E31C  |     | register x                       |             |          |           |             |
|               |     | 0xE000E400   |     | Interrupt                        |             |          | priority  |             |
| PFIC_IPRIORx  |     |              | RW  |                                  |             |          |           | 0x00000000  |
|               |     | -0xE000E43C  |     | configuration register           |             |          |           |             |
PFIC_CSTAR0  0xE000E780  RO  Core status register 0  0x00000000
Event Enable Register
|            |     |             |     | Note:   | Supported  |       | by  some     |             |
| ---------- | --- | ----------- | --- | ------- | ---------- | ----- | ------------ | ----------- |
| PFIC_EENR  |     | 0xE000EC80  | RW  |         |            |       |              | 0xFFFFFFFF  |
|            |     |             |     | chips,  | see        | chip  | manual  for  |             |
specific implementation.
Event pending register
|           |     |             |          | Note:   | Supported  |       | by  some     |             |
| --------- | --- | ----------- | -------- | ------- | ---------- | ----- | ------------ | ----------- |
| PFIC_EPR  |     | 0xE000EC84  | R0/RW1Z  |         |            |       |              | 0x00000000  |
|           |     |             |          | chips,  | see        | chip  | manual  for  |             |
specific implementation.
Event wake-up Register
|             |     |             |     | Note:   | Supported  |       | by  some     |             |
| ----------- | --- | ----------- | --- | ------- | ---------- | ----- | ------------ | ----------- |
| PFIC_EWUPR  |     | 0xE000EC88  | RO  |         |            |       |              | 0x00000000  |
|             |     |             |     | chips,  | see        | chip  | manual  for  |             |
specific implementation.
PFIC_SCTLR  0xE000ED10  RW  System control register  0x00000000
Note: 1. NMI, EXC, ECALL-M, ECALL-U, BREAKPOINT are always enabled by default.
    2. ECALL-M, ECALL-U, and BREAKPOINT are a case of EXC.
3.  NMI,  EXC,  ECALL-M, ECALL-U, BREAKPOINT support  interrupt  pending clear  and  setting
operation, but not interrupt enable clear and setting operation.

Each register is described as follows:
Interrupt enable status and interrupt pending status registers (PFIC_ISR<0-7>/PFIC_IPR<0-7>)
|       | Name       | Access address  | Access  |                                      | Description  |         |            | Reset value   |
| ----- | ---------- | --------------- | ------- | ------------------------------------ | ------------ | ------- | ---------- | ------------- |
|       |            |                 |         | Interrupt                            | 0-31         | enable  | status     | For V3A:      |
|       |            |                 |         | register, a total of 32 status bits  |              |         |            | 0x0000000C    |
|       | PFIC_ISR0  | 0xE000E000      | RO      |                                      |              |         |            |               |
|       |            |                 |         | [n],                                 | indicating   | #n      | interrupt  | For V3B/C/F:  |
|       |            |                 |         | enable status                        |              |         |            | 0x0000032C    |
| V1.5  |            |                 |         | 10                                   |              |         |            |               |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
Note: NMI and EXC are enabled
by default
|            |             | Interrupt  | 32-63  enable  status  |             |
| ---------- | ----------- | ---------- | ---------------------- | ----------- |
| PFIC_ISR1  | 0xE000E004  | RO         |                        | 0x00000000  |
register, total 32 status bits
| …   | …   | …  …  |     | …   |
| --- | --- | ----- | --- | --- |
Interrupt 224-255 enable status
| PFIC_ISR7  | 0xE000E01C  | RO  |     | 0x00000000  |
| ---------- | ----------- | --- | --- | ----------- |
register, total 32 status bits
|     |     | Interrupt  | 0-31  pending  status  |     |
| --- | --- | ---------- | ---------------------- | --- |
register, a total of 32 status bits
| PFIC_IPR0  | 0xE000E020  | RO  |     | 0x00000000  |
| ---------- | ----------- | --- | --- | ----------- |
[n], indicating the pending status
of interrupt #n
|            |             | Interrupt  | 32-63  pending  status  |             |
| ---------- | ----------- | ---------- | ----------------------- | ----------- |
| PFIC_IPR1  | 0xE000E024  | RO         |                         | 0x00000000  |
registers, 32 status bits in total
| …   | …   | …  …  |     | …   |
| --- | --- | ----- | --- | --- |
Interrupt 244-255 pending status
| PFIC_IPR7  | 0xE000E03C  | RO  |     | 0x00000000  |
| ---------- | ----------- | --- | --- | ----------- |
register, 32 status bits in total
2 sets of registers are used to enable and de-enable the corresponding interrupts.

Interrupt enable setting and clear registers (PFIC_IENR<0-7>/PFIC_IRER<0-7>)
| Name  | Access address  | Access  | Description  | Reset value  |
| ----- | --------------- | ------- | ------------ | ------------ |
Interrupt  0-31  enable  setting
register, a total of 32 setting bits
[n],  for  interrupt  #n  enable
| PFIC_IENR0  | 0xE000E100  | WO  |     | 0x00000000  |
| ----------- | ----------- | --- | --- | ----------- |
setting
Note: NMI and EXC are enabled
by default
Interrupt  32-63  enable  setting
| PFIC_IENR1  | 0xE000E104  | WO  |     | 0x00000000  |
| ----------- | ----------- | --- | --- | ----------- |
register, total 32 setting bits
| …   | …   | …  …  |     | …   |
| --- | --- | ----- | --- | --- |
Interrupt 224-255 enable setting
| PFIC_IENR7  | 0xE000E11C  | WO  |     | 0x00000000  |
| ----------- | ----------- | --- | --- | ----------- |
register, total 32 setting bits
| -   | -   | -          | -                    | -   |
| --- | --- | ---------- | -------------------- | --- |
|     |     | Interrupt  | 0-31  enable  clear  |     |
register, a total of 32 clear bits
PFIC_IRER0  0xE000E180  WO  [n], for interrupt #n enable clear  0x00000000
Note: NMI and EXC cannot be
operated
|             |             | Interrupt  | 32-63  enable  clear  |             |
| ----------- | ----------- | ---------- | --------------------- | ----------- |
| PFIC_IRER1  | 0xE000E184  | WO         |                       | 0x00000000  |
register, total 32 clear bits
| …   | …   | …  …  |     | …   |
| --- | --- | ----- | --- | --- |
Interrupt 244-255 enable clear
| PFIC_IRER7  | 0xE000E19C  | WO  |     | 0x00000000  |
| ----------- | ----------- | --- | --- | ----------- |
register, total 32 clear bits
2 sets of registers are used to enable and de-enable the corresponding interrupts.
Note: When using registers to mask any interrupt or using CSR registers to mask global interrupts, add a
| V1.5  |     | 11  |     |     |
| ----- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
'fence.i' instruction to synchronize between core control state and interrupt enable state.

Interrupt pending setting and clear registers (PFIC_IPSR<0-7>/PFIC_IPRR<0-7>)
|     | Name  | Access address  |     | Access  |     |     | Description  |     |     | Reset value  |     |
| --- | ----- | --------------- | --- | ------- | --- | --- | ------------ | --- | --- | ------------ | --- |
Interrupt 0-31 pending setting register, 32
PFIC_IPSR0  0xE000E200  WO  setting bits [n], for interrupt #n pending  0x00000000
setting
Interrupt 32-63 pending setup register,
|     | PFIC_IPSR1  | 0xE000E204  |     | WO  |     |     |     |     |     | 0x00000000  |     |
| --- | ----------- | ----------- | --- | --- | --- | --- | --- | --- | --- | ----------- | --- |
total 32 setup bits
|     | …           |             | …   | …   | …          |     |          |          |          |             | …   |
| --- | ----------- | ----------- | --- | --- | ---------- | --- | -------- | -------- | -------- | ----------- | --- |
|     |             |             |     |     | Interrupt  |     | 224-255  | pending  | setting  |             |     |
|     | PFIC_IPSR7  | 0xE000E21C  |     | WO  |            |     |          |          |          | 0x00000000  |     |
register, 32 setting bits in total
|     | -   |     | -   |     | -   |     |     | -   |     |     | -   |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
Interrupt 0-31 pending clear register, a
PFIC_IPRR0  0xE000E280  WO  total of 32 clear bits [n], for interrupt #n  0x00000000
pending clear
|     |             |             |     |     | Interrupt  |     | 32-63  pending  | clear  | register,  |             |     |
| --- | ----------- | ----------- | --- | --- | ---------- | --- | --------------- | ------ | ---------- | ----------- | --- |
|     | PFIC_IPRR1  | 0xE000E284  |     | WO  |            |     |                 |        |            | 0x00000000  |     |
total 32 clear bits
|     | …   |     | …   | …   | …   |     |     |     |     |     | …   |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
Interrupt 244-255 pending clear register,
|     | PFIC_IPRR7  | 0xE000E29C  |     | WO  |     |     |     |     |     | 0x00000000  |     |
| --- | ----------- | ----------- | --- | --- | --- | --- | --- | --- | --- | ----------- | --- |
total 32 clear bits
When the microprocessor enables an interrupt, it can be set directly through the interrupt pending register to
trigger into the interrupt. Use the interrupt pending clear register to clear the pending trigger.

Interrupt activation status register (PFIC_IACTR<0-7>)
|     | Name  |     | Access address  |     | Access  |            |     | Description      |      | Reset value  |     |
| --- | ----- | --- | --------------- | --- | ------- | ---------- | --- | ---------------- | ---- | ------------ | --- |
|     |       |     |                 |     |         | Interrupt  |     | 0-31  activates  | the  |              |     |
status register with 32 status
|     | PFIC_IACTR0  |     | 0xE000E300  |     | RO  |       |       |             |       | 0x00000000  |     |
| --- | ------------ | --- | ----------- | --- | --- | ----- | ----- | ----------- | ----- | ----------- | --- |
|     |              |     |             |     |     | bits  | [n],  | indicating  | that  |             |     |
interrupt #n is being executed
|     |     |     |     |     |     | Interrupt  |     | 32-63  | activation  |     |     |
| --- | --- | --- | --- | --- | --- | ---------- | --- | ------ | ----------- | --- | --- |
PFIC_IACTR1  0xE000E304  RO  status registers, 32 status bits in  0x00000000
total
|     | …   |     | …   |     | …   | …          |     |          |             |     | …   |
| --- | --- | --- | --- | --- | --- | ---------- | --- | -------- | ----------- | --- | --- |
|     |     |     |     |     |     | Interrupt  |     | 224-255  | activation  |     |     |
PFIC_IACTR7  0xE000E31C  RO  status register, total 32 status  0x00000000
bits
Each interrupt has an active status bit that is set up when the interrupt is entered and cleared by hardware when
mret returns.

Interrupt priority and priority threshold registers (PFIC_IPRIOR<0-7>/PFIC_ITHRESDR)
|     | Name  |     | Access address  |     | Access  |     |     | Description  |     | Reset value  |     |
| --- | ----- | --- | --------------- | --- | ------- | --- | --- | ------------ | --- | ------------ | --- |
Interrupt 0 priority configuration.
|     | PFIC_IPRIOR0  |     | 0xE000E400  |     | RW  | V3A/V3V:  |     |     |     |     | 0x00  |
| --- | ------------- | --- | ----------- | --- | --- | --------- | --- | --- | --- | --- | ----- |
[7:4]: Priority control bits
| V1.5  |     |     |     |     |     | 12  |     |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | ------------------- |
If the configuration is not nested,
no preemption bit
If nesting is configured, bit7 is the
preempted bit.
[3:0]: Reserved, fixed to 0

V3B:
[7:6]: Priority control bits
If configuration is not nested, no
preemptive bits
If configured nested, all bits are
preempted, but up to two levels of
interrupts are allowed to occur
[5:0]: Reserved, fixed to 0

V3B/V3C/V3F:
[7:5]: Priority control bits
If configuration is not nested, no
preemptive bits
If configured nested, all bits are
preempted, but up to two levels of
interrupts are allowed to occur
[4:0]: Reserved, fixed to 0
|     |     |     | Note:  The  | smaller  | the  priority  |     |
| --- | --- | --- | ----------- | -------- | -------------- | --- |
value, the higher the priority. If
|     |     |     | the  same  | preemption  | priority  |     |
| --- | --- | --- | ---------- | ----------- | --------- | --- |
interrupt hangs at the same time,
|     |     |     | the  interrupt  | with  | the  higher  |     |
| --- | --- | --- | --------------- | ----- | ------------ | --- |
priority will be executed first.
Interrupt 1 priority setting, same
| PFIC_IPRIOR1  |     | 0xE000E401  | RW  |     |     | 0x00  |
| ------------- | --- | ----------- | --- | --- | --- | ----- |
function as PFIC_IPRIOR0
Interrupt 2 priority setting, same
| PFIC_IPRIOR2  |     | 0xE000E402  | RW  |     |     |     |
| ------------- | --- | ----------- | --- | --- | --- | --- |
function as PFIC_IPRIOR0
|                 | …   | …           | …          | …              |           | …     |
| --------------- | --- | ----------- | ---------- | -------------- | --------- | ----- |
|                 |     |             | Interrupt  | 254  priority  | setting,  |       |
| PFIC_IPRIOR254  |     | 0xE000E4FE  | RW         |                |           | 0x00  |
same function as PFIC_IPRIOR0
Interrupt 255 priority setting,
| PFIC_IPRIOR255  |     | 0xE000E4FF  | RW  |     |     | 0x00  |
| --------------- | --- | ----------- | --- | --- | --- | ----- |
same function as PFIC_IPRIOR0
|     | -   | -   | -          | -         |            | -   |
| --- | --- | --- | ---------- | --------- | ---------- | --- |
|     |     |     | Interrupt  | priority  | threshold  |     |
setting
V3A/V3V:
PFIC_ITHRESDR  0xE000E040  RW  [31:8]: Reserved, fixed to 0  0x00
[7:4]: Priority threshold
[3:0]: Reserved, fixed to 0

| V1.5  |     |     | 13  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

QingKeV3 Microprocessor Manual https://wch-ic.com
V3B:
[31:8]: Reserved, fixed to 0
[7:5]: Priority threshold
[4:0]: Reserved, fixed to 0
V3C/V3F:
[31:8]: Reserved, fixed to 0
[7:5]: Priority threshold
[4:0]: Reserved, fixed to 0
Note: For interrupts with priority
value ≥ threshold, the interrupt
service function is not executed
when a hang occurs, and when
this register is 0, it means the
threshold register is invalid.
Note: The number of interrupt nesting levels and preemptive priority bit width, preemptive priority bit width
configuration may be different, use need to refer to the specific chip manual.
Interrupt configuration register (PFIC_CFGR)
Name Access address Access Description Reset value
PFIC_CFGR 0xE000E048 RW Interrupt configuration register 0x00000000
For V3A, its bits are defined as:
Bit Name Access Description Reset value
Corresponding to different target control bits, the
corresponding security access identification data
needs to be written simultaneously in order to be
[31:16] KEYCODE WO modified, and the readout data is fixed to 0. 0
KEY1 = 0xFA05；
KEY2 = 0xBCAF；
KEY3 = 0xBEEF。
[15:8] Reserved RO Reserved 0
System reset (simultaneous writing to KEY3).
Auto clear 0.
7 SYSRESET WO Writing 1 is valid, writing 0 is invalid. 0
Note: Same function as the PFIC_SCTLR register
SYSRESET bit.
PFIC module reset. Auto clear 0.
6 PFICRESET WO 0
Writing 1 is valid, writing 0 is invalid.
Exception interrupt pending clear (simultaneous
5 EXCRESET WO writing to KEY2) 0
Writing 1 is valid, writing 0 is invalid.
Exception interrupt pending setting (simultaneous
4 EXCSET WO 0
writing to KEY2)
V1.5 14

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
Writing 1 is valid, writing 0 is invalid.
|     |     |           |     |     | NMI  interrupt    | pending  | clear  | (simultaneous  |     |     |
| --- | --- | --------- | --- | --- | ----------------- | -------- | ------ | -------------- | --- | --- |
|     | 3   | NMIRESET  |     | WO  | writing to KEY2)  |          |        |                |     | 0   |
Writing 1 is valid, writing 0 is invalid.
|     |     |         |     |     | NMI  interrupt    | pending  | setting  | (Simultaneous  |     |     |
| --- | --- | ------- | --- | --- | ----------------- | -------- | -------- | -------------- | --- | --- |
|     | 2   | NMISET  |     | WO  | writing to KEY2)  |          |          |                |     | 0   |
Writing 1 is valid, writing 0 is invalid.
Interrupt nesting enable control.
|     | 1   | NESTCTRL  |     | RW  |     |     |     |     |     | 0   |
| --- | --- | --------- | --- | --- | --- | --- | --- | --- | --- | --- |
1: off; 0: on (synchronous writing to KEY1)
HPE enable control
|     | 0   | HWSTKCTRL  |     | RW  |     |     |     |     |     | 0   |
| --- | --- | ---------- | --- | --- | --- | --- | --- | --- | --- | --- |
1: off; 0: on (synchronous writing to KEY1)

For V3F/V3V, its bits are defined as:
| Name    |     | Access address  |     | Access  |         | Description  |             |               | Reset value  |     |
| ------- | --- | --------------- | --- | ------- | ------- | ------------ | ----------- | ------------- | ------------ | --- |
|         |     |                 |     |         | System  | reset  when  | 0xBEEF0080  | is  written,  |              |     |
| [31:0]  |     | SYSRST          |     | WO      |         |              |             |               |              | 0   |
other values are invalid.

Interrupt global status register (PFIC_GISR)
|     | Name  |     | Access address  |     | Access  |     | Description  |     | Reset value  |     |
| --- | ----- | --- | --------------- | --- | ------- | --- | ------------ | --- | ------------ | --- |
PFIC_GISR  0xE000E04C  RO  Interrupt global status register  0x00000000

Its folks are defined as.
|          | Bit  | Name      |     | Access  |           | Description  |     |     | Reset value  |     |
| -------- | ---- | --------- | --- | ------- | --------- | ------------ | --- | --- | ------------ | --- |
| [31:16]  |      | Reserved  |     | RO      | Reserved  |              |     |     |              | 0   |
Core status register, used to get the operational
status of the core:
00: Running;
| [15:14]  |     | EXSTATE  |     | RO  | 01: Light sleep;   |     |     |     |     | 0   |
| -------- | --- | -------- | --- | --- | ------------------ | --- | --- | --- | --- | --- |
10: Deep sleep;
11: Deep sleep (locked).
Note: This bit is only valid for V3B/C/F/V.
Whether the processor is currently in a locked
state:
|     | 13  | LOCKSTA  |     | RO  | 1: Locked state;  |     |     |     |     | 0   |
| --- | --- | -------- | --- | --- | ----------------- | --- | --- | --- | --- | --- |
0: Non-locked state.
Note: This bit is only valid for the V3B/C/F/V.
Whether the processor is currently in debug state:
1: Debug state;
|     | 12  | DBGMODE  |     | RO  |     |     |     |     |     | 0   |
| --- | --- | -------- | --- | --- | --- | --- | --- | --- | --- | --- |
0: Non-debug state.
Note: This bit is only valid for the V3B/C/F/V.
Global interrupt enable:
1: Enable interrupt;
|     | 11  | GLOBLIE  |     | RO  |     |     |     |     |     |     |
| --- | --- | -------- | --- | --- | --- | --- | --- | --- | --- | --- |
0: Disable interrupt.
Note: This bit is only valid for the V3B/C/F/V.
|       | 10  | Reserved  |     | RO  | Reserved  |     |     |     |     | 0   |
| ----- | --- | --------- | --- | --- | --------- | --- | --- | --- | --- | --- |
| V1.5  |     |           |     |     |           | 15  |     |     |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
Whether an interrupt is currently pending.
|     | 9   | GPENDSTA  |     | RO  |     |     |     |     |     | 0   |
| --- | --- | --------- | --- | --- | --- | --- | --- | --- | --- | --- |
1: Yes;     0: No.
Whether an interrupt is currently being executed.
|     | 8   | GACTSTA  |     | RO  |     |     |     |     |     | 0   |
| --- | --- | -------- | --- | --- | --- | --- | --- | --- | --- | --- |
1: Yes;     0: No.
Current interrupt nesting status.
0x03: in level 2 interrupt.
|     | [7:0]  | NESTSTA  |     | RO  0x01: in level 1 interrupt.  |     |     |     |     |     | 0   |
| --- | ------ | -------- | --- | -------------------------------- | --- | --- | --- | --- | --- | --- |
0x00: no interrupts occur.
Other: Impossible situation.

VTF ID base address and offset address registers (PFIC_VTFBADDRR/PFIC_VTFADDRR<0-3>)
|     |     | Name  | Access address  |     | Access  |     | Description  |     | Reset value  |     |
| --- | --- | ----- | --------------- | --- | ------- | --- | ------------ | --- | ------------ | --- |
[31:28]: High 4 bits of the
target address of VTF
PFIC_VTFBADDRR  0xE000E044  RW  [27:0]: Reserved  0x00000000
This register is valid only for
V3A.
[31:24]: Number of VTF 3
[23:16]: Number of VTF 2
[15:8]: Number of VTF 1
|     | PFIC_VTFIDR  |     | 0xE000E050  |     | RW  |     |     |     | 0x00000000  |     |
| --- | ------------ | --- | ----------- | --- | --- | --- | --- | --- | ----------- | --- |
[7:0]: Number of VTF 0
This register is valid only for
V3B/C/F/V.
|     |     | -   |     | -   |     | -   | -   |     |     | -   |
| --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
V3A:
|     |     |     |     |     |     | [31:24]:  | VTF  0  interrupt  |     |     |     |
| --- | --- | --- | --- | --- | --- | --------- | ------------------ | --- | --- | --- |
number
[23:0]: the low 24 bits of the
VTF target address, of which
the low 20 bits are configured
For V3A:
|     |     |     |     |     |     | to  be  | valid  and  [23:20]  | is  |     |     |
| --- | --- | --- | --- | --- | --- | ------- | -------------------- | --- | --- | --- |
0x00000000
|     | PFIC_VTFADDRR0  |     | 0xE000E060  |     | RW  | fixed to 0.  |     |     |     |     |
| --- | --------------- | --- | ----------- | --- | --- | ------------ | --- | --- | --- | --- |
For V3B/C/F/V:

0xXXXXXXXX
V3B/C/F/V:
[31:1]: VTF 0 address, 2-byte
aligned
[0]:
1: Enable VTF 0 channel
0: Disable
V3A:
|     |     |     |     |     |     | [31:24]:  | VTF  1  interrupt  |     |     |     |
| --- | --- | --- | --- | --- | --- | --------- | ------------------ | --- | --- | --- |
For V3A:
number
0x00000000
PFIC_VTFADDRR1  0xE000E064  RW  [23:0]: The low 24 bits of the
For V3B/C/F/V:
VTF target address, of which
0xXXXXXXXX
the low 20 bits are configured
|       |     |     |     |     |     | to  be  | valid  and  [23:20]  | is  |     |     |
| ----- | --- | --- | --- | --- | --- | ------- | -------------------- | --- | --- | --- |
| V1.5  |     |     |     |     |     | 16      |                      |     |     |     |

QingKeV3 Microprocessor Manual https://wch-ic.com
fixed to 0.
V3B/C/F/V:
[31:1]: VTF 1 address, 2-byte
aligned
[0]:
1: Enable VTF 1 channel
0: Disable
V3A:
[31:24]: VTF 2 interrupt
number
[23:0]: the low 24 bits of the
VTF target address, of which
the low 20 bits are configured
For V3A:
to be valid and [23:20] is
0x00000000
PFIC_VTFADDRR2 0xE000E068 RW fixed to 0.
For V3B/C/F/V:
0xXXXXXXXX
V3B/C/F/V:
[31:1]: VTF 2 address, 2-byte
aligned
[0]:
1: Enable VTF 2 channel
0: Disable
V3A:
[31:24]: VTF 3 interrupt
number
[23:0]: the low 24 bits of the
VTF target address, of which
the low 20 bits are configured
For V3A:
to be valid and [23:20] is
0x00000000
PFIC_VTFADDRR3 0xE000E06C RW fixed to 0.
For V3B/C/F/V:
0xXXXXXXXX
V3B/C/F/V:
[31:1]: VTF 3 address, 2-byte
aligned
[0]:
1: Enable VTF 3 channel
0: Disable
Core status register (PFIC_CSTAR0)
Name Access address Access Description Reset value
PFIC_CSTAR0 0xE000E780 RO Core status register 0 0x00000000
Its folks are defined as:
Name Access address Access Description Reset value
V1.5 17

| QingKeV3 Microprocessor Manual  |           |     |           |     |     | https://wch-ic.com  |
| ------------------------------- | --------- | --- | --------- | --- | --- | ------------------- |
| [31:16]                         | Reserved  | RO  | Reserved  |     |     | 0                   |
Core C0 status register, used to get
the running status of core C0:
00: Running;
| [15:14]  | CPU_ex_state_0  | RO  |     |     |     | 0   |
| -------- | --------------- | --- | --- | --- | --- | --- |
01: Light sleep;
10: Deep sleep;
11: Deep sleep (locked).
Core C0 locked state register, used
|     |                |     | to  query       | whether  core  | C0  is  | in  |
| --- | -------------- | --- | --------------- | -------------- | ------- | --- |
| 13  | CPU_lock_up_0  | RO  | locked state:   |                |         | 0   |
1: Locked;
0: Non-locked.
|     |     |     | Kernel  | C0  debug  mode  | register,  |     |
| --- | --- | --- | ------- | ---------------- | ---------- | --- |
used to query whether core C0 is in
| 12  | CPU_dbg_mode_0  | RO  | debug mode:   |     |     | 0   |
| --- | --------------- | --- | ------------- | --- | --- | --- |
1: Core is in debug mode;
0: Core is in non-debug mode.
|     |     |     | Core  C0  | global  interrupt  | enable  |     |
| --- | --- | --- | --------- | ------------------ | ------- | --- |
register, used to query whether the
core C0 global interrupt is enabled
| 11  | CPU_globl_ie_0  | RO  |     |     |     | 0   |
| --- | --------------- | --- | --- | --- | --- | --- |
or not:
1: Core global interrupt enable is on;
0: Core global interrupt enable is off.
| 10  | Reserved  | RO  | Reserved  |                     |       | 0   |
| --- | --------- | --- | --------- | ------------------- | ----- | --- |
|     |           |     | Core  C0  | interrupt  pending  | flag  |     |
register, used to query whether there
are unprocessed interrupts in core
C0:
| 9   | CPU_irq_pend_0  | RO  |     |     |     | 0   |
| --- | --------------- | --- | --- | --- | --- | --- |
1: There are unprocessed interrupts
in the core;
|     |     |     | 0:  There  | are  no  | unprocessed  |     |
| --- | --- | --- | ---------- | -------- | ------------ | --- |
interrupts in the core.
|     |     |     | Core  C0  | interrupt  | active  flag  |     |
| --- | --- | --- | --------- | ---------- | ------------- | --- |
register, used to query whether core
| 8   | CPU_irq_active_0  | RO  | C0 is processing interrupts   |     |     | 0   |
| --- | ----------------- | --- | ----------------------------- | --- | --- | --- |
1: Core is processing interrupts;
0: Core is not processing interrupts.
|     |     |     | Core  C0  | interrupt  nested  | status  |     |
| --- | --- | --- | --------- | ------------------ | ------- | --- |
register, used to query the nested
[7:0]  CPU_nest_sta_0  RO  status of kernel C0 interrupts. Same  0
|     |     |     | as  the  | INEST_CTLR  | register  |     |
| --- | --- | --- | -------- | ----------- | --------- | --- |
nest_sta bit of the CSR register.

Event enable register (PFIC_EENR)
| Name  | Access address  | Access  |     | Description  |     | Reset value  |
| ----- | --------------- | ------- | --- | ------------ | --- | ------------ |
| V1.5  |                 |         | 18  |              |     |              |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
PFIC_EENR  0xE000EC80  RW  Event enable register  0x00000000

Some chips support it, see the chip manual for the implementation, and its folks are defined as:
|     | Name  | Access address  |     | Access  |     | Description  |     |     |     | Reset value  |
| --- | ----- | --------------- | --- | ------- | --- | ------------ | --- | --- | --- | ------------ |
31#-0# event wakeup enable:
1: Allow the corresponding event to wake
up the kernel during sleep;
0: Block the corresponding event and do
not allow waking up the kernel when this
event occurs.
Up to 32 event sources are supported, of
which 0-7 is level triggered (high active)
and 8-31 is pulse triggered:
0#: Core pause request;
1#: Core interrupt request;
7#-2#: Reserved;
|     |         |          |     |     | 8#: Core entry interrupt;  |     |     |     |     | 0xFFFFFFF |
| --- | ------- | -------- | --- | --- | -------------------------- | --- | --- | --- | --- | --------- |
|     | [31:0]  | EVENTEN  |     | RW  |                            |     |     |     |     |           |
|     |         |          |     |     | 9#: Core exit interrupt;   |     |     |     |     | F         |
10#: A new interrupt is pending; (For
shared interrupts, it is not distinguished
whether the interrupt is routed to the local
core.)
11#: Core sends event; (Local core sends
events)
|     |     |     |     |     | 12#:  Core  | receive  | events;(Other  |     | core  |     |
| --- | --- | --- | --- | --- | ----------- | -------- | -------------- | --- | ----- | --- |
sends events)
|     |     |     |     |     | 13#:  | The  | system  | receives  |     |     |
| --- | --- | --- | --- | --- | ----- | ---- | ------- | --------- | --- | --- |
events;(system-configured events such as
GPIO rollover events…)
31#-14#: Reserved.

Event pending register (PFIC_EPR)
|     | Name  | Access address  |     | Access  |     |     | Description  |     | Reset value  |     |
| --- | ----- | --------------- | --- | ------- | --- | --- | ------------ | --- | ------------ | --- |
PFIC_EPR  0xE000EC84  RW  Event pending register  0x00000000

Some chips support it, see the chip manual for the implementation, and its folks are defined as:
|     | Name  | Access address  | Access  |     |     | Description  |     |     | Reset value  |     |
| --- | ----- | --------------- | ------- | --- | --- | ------------ | --- | --- | ------------ | --- |
31#-8# Event pending status, write 1 to
clear:
|     |         |             |       | 1:  | The  | corresponding  | event  | occurs  |     |     |
| --- | ------- | ----------- | ----- | --- | ---- | -------------- | ------ | ------- | --- | --- |
|     | [31:8]  | EVENT_PEND  | RW1Z  |     |      |                |        |         |     | 0   |
without being processed;
|     |     |     |     | 0:  | The  corresponding  |     | event  | does  not  |     |     |
| --- | --- | --- | --- | --- | ------------------- | --- | ------ | ---------- | --- | --- |
occur.
7#-0# Event pending status, not clearable:
|       | [7:0]  | EVENT_PEND  | RO  |     |                     |     |        |           |     | 0   |
| ----- | ------ | ----------- | --- | --- | ------------------- | --- | ------ | --------- | --- | --- |
|       |        |             |     | 1:  | The  corresponding  |     | event  | occurred  |     |     |
| V1.5  |        |             |     |     | 19                  |     |        |           |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
without being processed;
0: The corresponding event did not occur.

Event wakeup register (PFIC_EWUPR)
|     | Name  | Access address  |     | Access  |     | Description  |     |     |     | Reset value  |
| --- | ----- | --------------- | --- | ------- | --- | ------------ | --- | --- | --- | ------------ |
PFIC_EENR  0xE000EC88  RO  Event wakeup register  0x00000000

Some chips support it, see the chip manual for the implementation, and its folks are defined as:
|     | Name  | Access address  |     | Access  |     |     | Description  |     |     | Reset value  |
| --- | ----- | --------------- | --- | ------- | --- | --- | ------------ | --- | --- | ------------ |
31#-0# event wake-up registers:
1: The corresponding event wakes up
|     | [31:0]  | EVENT_WUP  |     | RO  | the core from sleep;   |     |     |     |     | 0   |
| --- | ------- | ---------- | --- | --- | ---------------------- | --- | --- | --- | --- | --- |
0: The corresponding event is not the
event that wakes up the core this time.

System control register (PFIC_SCTLR)
|     | Name  | Access address  |     | Access  |     |     | Description  |     |     | Reset value  |
| --- | ----- | --------------- | --- | ------- | --- | --- | ------------ | --- | --- | ------------ |
PFIC_SCTLR  0xE000ED10  RW  System control register  0x00000000

Each of them is defined as follows.
|     | Bit  | Name  | Access  |     |     | Description  |     |     |     | Reset value  |
| --- | ---- | ----- | ------- | --- | --- | ------------ | --- | --- | --- | ------------ |
System reset, auto clear 0. Write 1 is
valid, write 0 is invalid.
|     | 31  | SYSRESET  |     | WO  |        |       |          |              |      | 0   |
| --- | --- | --------- | --- | --- | ------ | ----- | -------- | ------------ | ---- | --- |
|     |     |           |     |     | Note:  | This  | bit  is  | only  valid  | for  |     |
V3B/C/F/V
|     | [30:24]  | Reserved  |     | RO  | Reserved  |     |     |     |     | 0   |
| --- | -------- | --------- | --- | --- | --------- | --- | --- | --- | --- | --- |
Core ID register, used to get the core ID
of the core reading this register:
|     | [23:16]  | HART_ID  |     | RO  | 1: this core is C1;   |     |     |     |     | 0   |
| --- | -------- | -------- | --- | --- | --------------------- | --- | --- | --- | --- | --- |
0: this core is C0.
Note: This bit is only valid for V3F.
|     | [15:6]  | Reserved  |     | RO  | Reserved                                |     |     |     |     | 0   |
| --- | ------- | --------- | --- | --- | --------------------------------------- | --- | --- | --- | --- | --- |
|     | 5       | SETEVENT  |     | WO  | Set the event to wake up the WFE case.  |     |     |     |     | 0   |
When an event occurs or interrupts a
pending state, the system can be woken
up from after the WFE instruction, or if
the WFE instruction is not executed, the
system will be woken up immediately
|     |     |            |     |     | after  the  | next  | execution  |     | of  the  |     |
| --- | --- | ---------- | --- | --- | ----------- | ----- | ---------- | --- | -------- | --- |
|     | 4   | SEVONPEND  |     | RW  |             |       |            |     |          | 0   |
instruction.
|     |     |     |     |     | 1:  Enabled  | events     | and  | all  interrupts  |      |     |
| --- | --- | --- | --- | --- | ------------ | ---------- | ---- | ---------------- | ---- | --- |
|     |     |     |     |     | (Including   | unenabled  |      | interrupts)      | can  |     |
wake up the system.
|     |     |     |     |     | 0:  Only  | enabled  | events  | and  | enabled  |     |
| --- | --- | --- | --- | --- | --------- | -------- | ------- | ---- | -------- | --- |
interrupts can wake up the system.
| V1.5  |     |     |     |     | 20  |     |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
Execute the WFI command as if it were
a WFE.
|     | 3   | WFITOWFE  |     | RW  |     | 1: Treat the subsequent WFI instruction  |     |     |     |     |     | 0   |
| --- | --- | --------- | --- | --- | --- | ---------------------------------------- | --- | --- | --- | --- | --- | --- |
as a WFE instruction.
0: No effect.
Low power mode of the control system.
|     | 2   | SLEEPDEEP  |     | RW  |     |     |     |     |     |     |     | 0   |
| --- | --- | ---------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |
1: deepsleep 0: sleep
System status after control leaves the
|     |     | SLEEPONEXI |     |     |     | interrupt service program.            |     |     |     |     |     |     |
| --- | --- | ---------- | --- | --- | --- | ------------------------------------- | --- | --- | --- | --- | --- | --- |
|     | 1   |            |     | RW  |     |                                       |     |     |     |     |     | 0   |
|     |     |            | T   |     |     | 1: The system enters low-power mode.  |     |     |     |     |     |     |
0: The system enters the main program.
|     | 0   | Reserved  |     | RO  |     | Reserved  |     |     |     |     |     | 0   |
| --- | --- | --------- | --- | --- | --- | --------- | --- | --- | --- | --- | --- | --- |

3.2 Interrupt-related CSR Registers
In addition, the following CSR registers also have a significant impact on the processing of interrupts.
Interrupt system control register (intsyscr)
This register is not valid for V3A only:
|     | Name  |     | CSR Address  |     | Access  |     |     | Description  |     |     | Reset value  |     |
| --- | ----- | --- | ------------ | --- | ------- | --- | --- | ------------ | --- | --- | ------------ | --- |
intsyscr  0x804  URW  Interrupt system control register  0x0000E002

Its folks are defined as:
|     | Bit  |     | Name  |     | Access  |     | Description  |     |     |     | Reset value  |     |
| --- | ---- | --- | ----- | --- | ------- | --- | ------------ | --- | --- | --- | ------------ | --- |
0: This register can be read and written in
user mode;
1: This register can only be read and
|     | 31  |     | LOCK  |     | URO  |     |     |     |     |     |     | 0   |
| --- | --- | --- | ----- | --- | ---- | --- | --- | --- | --- | --- | --- | --- |
written in machine mode.
Note: This configuration bit is valid from
version 1.0 onwards.
|     | [30:6]  |     | Reserved  |     | URO  | Reserved           |     |                |     |        |     | 0x380  |
| --- | ------- | --- | --------- | --- | ---- | ------------------ | --- | -------------- | --- | ------ | --- | ------ |
|     |         |     |           |     |      | Global  interrupt  |     | and  hardware  |     | stack  |     |        |
shutdown are enabled.
Note: This bit is often used in real-time
operating systems. When the context is
switched during an interrupt, setting this
|     | 5   | GIHWSTKNEN  |     |     | URW1  |     |     |     |     |     |     | 0   |
| --- | --- | ----------- | --- | --- | ----- | --- | --- | --- | --- | --- | --- | --- |
bit can turn off the global interrupt and
|     |     |     |     |     |     | push  the        | hardware  | stack.         | When      |       | the  |     |
| --- | --- | --- | --- | --- | --- | ---------------- | --------- | -------------- | --------- | ----- | ---- | --- |
|     |     |     |     |     |     | context  switch  |           | is  completed  |           | and   | the  |     |
|     |     |     |     |     |     | interrupt        | returns,  | the            | hardware  | will  |      |     |
automatically clear this bit.
|     | 4   |     | Reserved  |     | URO  | Reserved       |     |               |             |     |     | 0   |
| --- | --- | --- | --------- | --- | ---- | -------------- | --- | ------------- | ----------- | --- | --- | --- |
|     |     |     |           |     |      | Configuration  |     | of  priority  | preemption  |     |     |     |
bits:
|     | [3:2]  |     | PMTCFG  |     | URW  |     |     |     |     |     |     | 0   |
| --- | ------ | --- | ------- | --- | ---- | --- | --- | --- | --- | --- | --- | --- |
00: The number of preemption bits is 0;
01: The number of preemption bits is 1;
| V1.5  |     |     |     |     |     | 21  |     |     |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
10: The number of preemption bits is 2;
11: The number of preemption bits is 3;
Note: This configuration bit is valid after
1.0.
The interrupt nesting function is enabled,
and the fixed value is 1:
0: Disable;
1: Enable.
|     | 1   | INESTEN  | URW  |        |                  |          |        | 1   |
| --- | --- | -------- | ---- | ------ | ---------------- | -------- | ------ | --- |
|     |     |          |      | Note:  | 1.  The  actual  | nesting  | level  | is  |
controlled by NEST_LVL in CSR 0xBC1;
2. Only versions after 1.0 can be
written.
Hardware stack enable:
0: Hardware stack pressing function is
|     | 0   | HWSTKEN  | URW  | disabled;  |     |     |     | 0   |
| --- | --- | -------- | ---- | ---------- | --- | --- | --- | --- |
1: Hardware stack pressing function is
enabled.

Machine mode exception base address register (mtvec)
|     | Name  | CSR Address  | Access  |     | Description  |     |     | Reset value  |
| --- | ----- | ------------ | ------- | --- | ------------ | --- | --- | ------------ |
mtvec  0x305  MRW  Exception base address register  0x00000000
Its folks are defined as.
|     | Bit  | Name  | Access  |     | Description  |     |     | Reset value  |
| --- | ---- | ----- | ------- | --- | ------------ | --- | --- | ------------ |
Interrupt vector table base address, where
|     | [31:2]  | BASEADDR  | MRW  |     |     |     |     | 0   |
| --- | ------- | --------- | ---- | --- | --- | --- | --- | --- |
bits [9:2] are fixed to 0.
Interrupt vector table recognition mode:
|     |     |     |     | 0:  Identify  | by  jump              | instruction,  | with  |     |
| --- | --- | --- | --- | ------------- | --------------------- | ------------- | ----- | --- |
|     |     |     |     | limited       | scope,  and  support  | non-jump      |       |     |
instruction;
|     | 1   | MODE1  | MRO  |     |     |     |     | 0   |
| --- | --- | ------ | ---- | --- | --- | --- | --- | --- |
1: Identify by absolute address, support
full range, but must jump.
|     |     |     |     | Note:  This  | bit  is  | only  valid  | for  |     |
| --- | --- | --- | --- | ------------ | -------- | ------------ | ---- | --- |
V3B/C/F/V.
Interrupt or exception entry address mode
selection.
|     | 0   | MODE0  | MRW  | 0: Use of the uniform entry address.  |                |     |            | 0   |
| --- | --- | ------ | ---- | ------------------------------------- | -------------- | --- | ---------- | --- |
|     |     |        |      | 1:  Address                           | offset  based  | on  | interrupt  |     |
number *4.
For MCUs with V3 series microprocessors, MODE0 is configured to be 1 by default in the startup file, and
the entries for exceptions or interrupts are offset according to the interrupt number *4. Note that the V3A
microprocessor stores a jump instruction at the vector table, while the V3B/C/F/V microprocessor can be either
a jump instruction or use the absolute address of the interrupt function, which is configured as an absolute
address in the default startup file.

Microprocessor configuration register (corecfgr)
| V1.5  |     |     |     | 22  |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
This register is invalid for V3A:
|     | Name      | CSR Address  | Access  |                 | Description  |                |     |                   | Reset value  |     |
| --- | --------- | ------------ | ------- | --------------- | ------------ | -------------- | --- | ----------------- | ------------ | --- |
|     |           |              |         | Microprocessor  |              | configuration  |     | V3B/C:0x00000001  |              |     |
|     | corecfgr  | 0xBC0        | MRW     |                 |              |                |     |                   |              |     |
|     |           |              |         | register        |              |                |     | V3F:0x12370000    |              |     |

Its folks are defined as.
|     | Bit  | Name  | Access  |     |                 | Description        |     |           |     | Reset value  |
| --- | ---- | ----- | ------- | --- | --------------- | ------------------ | --- | --------- | --- | ------------ |
|     |      |       |         |     | Floating-point  | plus  instruction  |     | division  |     |              |
factor, default value is 1, that is, floating-
[31:28]  FADD_CLKDIV  MRW  point  plus  instruction  frequency  is  the  0x1
dominant frequency of 2 division.
Note: This bit is valid only for V3F.
|     |     |     |     |     | Floating-point  | multiply  | instruction  |     |     |     |
| --- | --- | --- | --- | --- | --------------- | --------- | ------------ | --- | --- | --- |
division factor, default value is 2, that is,
|     |          |              |      |     | floating-point  | multiply  | instruction  |     |     |      |
| --- | -------- | ------------ | ---- | --- | --------------- | --------- | ------------ | --- | --- | ---- |
|     | [27:24]  | FMUL_CLKDIV  | MRW  |     |                 |           |              |     |     | 0x2  |
frequency is the dominant frequency of 3
division.
Note: This bit is valid only for V3F.
|     |     |     |     |     | Floating-point  | multiply  | plus  instruction  |     |     |     |
| --- | --- | --- | --- | --- | --------------- | --------- | ------------------ | --- | --- | --- |
division factor, default value is 3, that is,
|     |          |              |      |     | floating-point  | multiply  | plus  instruction  |     |     |      |
| --- | -------- | ------------ | ---- | --- | --------------- | --------- | ------------------ | --- | --- | ---- |
|     | [23:20]  | FMAC_CLKDIV  | MRW  |     |                 |           |                    |     |     | 0x3  |
frequency is the dominant frequency of 4
division.
Note: This bit is valid only for V3F.
|     |     |     |     |     | Floating-point  | division  | instruction  |     |     |     |
| --- | --- | --- | --- | --- | --------------- | --------- | ------------ | --- | --- | --- |
division factor, default value is 7, that is,
|     |          |              |      |     | floating-point  | division  | instruction  |     |     |      |
| --- | -------- | ------------ | ---- | --- | --------------- | --------- | ------------ | --- | --- | ---- |
|     | [19:16]  | FDIV_CLKDIV  | MRW  |     |                 |           |              |     |     | 0x7  |
frequency is the dominant frequency of 8
division.
Note: This bit is valid only for V3F.
Core status error interrupt enable:
0: On status error, no NMI interrupt is
|     | 7   | CSTA_FAULT_IE  | MRW  |     | generated;      |              |            |     |     | 0   |
| --- | --- | -------------- | ---- | --- | --------------- | ------------ | ---------- | --- | --- | --- |
|     |     |                |      |     | 1:  On  status  | error,  NMI  | interrupt  |     | is  |     |
generated.
FENCE mask interrupt enable:
0: Do not clear the interrupt request when
|     | 6   | INT_FENCE  | MRW  |     | executing the fence instruction;   |                 |          |       |     | 0   |
| --- | --- | ---------- | ---- | --- | ---------------------------------- | --------------- | -------- | ----- | --- | --- |
|     |     |            |      |     | 1:  Clear                          | the  interrupt  | request  | when  |     |     |
executing the fence instruction.
MIE register mapping enable:
|     |     |     |     |     | 0:  CSR  address  | 0x800  | is  a  | read-only  |     |     |
| --- | --- | --- | --- | --- | ----------------- | ------ | ------ | ---------- | --- | --- |
5  IE_REMAP_EN  MRW  register and the return value is the value of  0
MSTATUS;
1: Bits 3 and 7 of CSR address 0x800 are
| V1.5  |     |     |     |     | 23  |     |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |         |               |          |          | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | ------- | ------------- | -------- | -------- | ------------------- | --- |
|                                 |     |     | mapped  | to  bit  MIE  | of  the  | MSTATUS  |                     |     |
register and bit MPIE of the MSTATUS
register, respectively.
| 4   | Reserved  | MRO  | Reserved  |     |     |     |     | 0   |
| --- | --------- | ---- | --------- | --- | --- | --- | --- | --- |
ROM area instruction loop acceleration
enable:
0: Turn off the cyclic acceleration function
in ROM area;
3  ROM_LOOP_ACC  MRW  1: Continuous instructions with loop body  0
within 128 bytes will be fully accelerated,
while those with loop body within 256
bytes will be partially accelerated.
Note: This bit is not valid for V3A/V3F.
ROM area instruction jump acceleration
enabled:
|     |               |      | 0:  Disable    | ROM  area  | instruction  | jump  |     |     |
| --- | ------------- | ---- | -------------- | ---------- | ------------ | ----- | --- | --- |
| 2   | ROM_JUMP_ACC  | MRW  | acceleration;  |            |              |       |     | 0   |
1: Enable instruction jump acceleration in
ROM area.
Note: This bit is not valid for V3A/V3F.
Fetching mode:
|     |     |     | 00:  Prefetch  | is  off.  | The  | instruction  |     |     |
| --- | --- | --- | -------------- | --------- | ---- | ------------ | --- | --- |
prefetch function is turned off to avoid
invalid instruction fetching operation, and
there is at most one valid instruction on the
CPU pipeline. This mode has the lowest
power consumption, and its performance
drops by about 2 ~ 3 times.
01: Prefetch Mode 1. When the instruction
prefetch function is turned on, the CPU
|     |     |     | will  continue  | to  access  |     | the  instruction  |     |     |
| --- | --- | --- | --------------- | ----------- | --- | ----------------- | --- | --- |
memory until the number of instructions to
|        |             |      | be  executed  | in  the  | internal  | instruction  |     |      |
| ------ | ----------- | ---- | ------------- | -------- | --------- | ------------ | --- | ---- |
| [1:0]  | FETCH_MODE  | MRW  |               |          |           |              |     | 0x1  |
buffer exceeds a certain number, or the
instruction buffer is full, and instruction
fetching will be suspended; (Failure of
|     |     |     | CPU  prediction  | will  | lead  | to  redundant  |     |     |
| --- | --- | --- | ---------------- | ----- | ----- | -------------- | --- | --- |
fetch operation, and in some cases, the
execution unit will introduce 0 ~ 2 cycles
of bubbles, and the performance of most
programs will not decrease obviously);
10: Reserved;
11: Prefetch Mode 2. When the instruction
prefetch function is turned on, the CPU
|     |     |     | will  continue  | to  access  |     | the  instruction  |     |     |
| --- | --- | --- | --------------- | ----------- | --- | ----------------- | --- | --- |
memory, and if the instruction buffer is
| V1.5  |     |     | 24  |     |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
full, the CPU will continue to retry the
|     |     |     |     | address.         | This  | mode     | has  the      | highest  |     |     |
| --- | --- | --- | --- | ---------------- | ----- | -------- | ------------- | -------- | --- | --- |
|     |     |     |     | performance      | and   | power    | consumption.  |          |     |     |
|     |     |     |     | CPU  prediction  |       | failure  | and  retry    | will     |     |     |
introduce redundant fetch operations and
|     |     |     |     | may  continue  |     | to  occupy  |     | memory  |     |     |
| --- | --- | --- | --- | -------------- | --- | ----------- | --- | ------- | --- | --- |
bandwidth. (For ROM area, retry means
|     |     |     |     | discontinuous  | address  |     | access,  | so  it  | is  |     |
| --- | --- | --- | --- | -------------- | -------- | --- | -------- | ------- | --- | --- |
|     |     |     |     | recommended    |          | to  | turn     |         | on  |     |
ROM_ACC_EN).
Floating-point  arithmetic  instructions  are  multi-cycle  instructions,  for  different  types  of  floating-point
instructions, the maximum running frequency constraints check each chip manual. Depending on the
processor's operating frequency, the user needs to configure the floating-point arithmetic crossover coefficients
so that the frequency after crossover is not higher than the maximum operating frequency shown in the table
above.
fxxx_- freq（max）  ≥ Core main frequency /  fxxx_- clkdiv

Interrupt nested control register (inestcr)
This register is invalid only for V3A:
|     | Name  | CSR Address  | Access  |     |     | Description  |     |     | Reset value  |     |
| --- | ----- | ------------ | ------- | --- | --- | ------------ | --- | --- | ------------ | --- |
inestcr  0xBC1  MRW  Interrupt nested control register  0x00000000

Its folks are defined as.
|     | Bit  | Name      | Access  |           | Description  |     |     |     | Reset value  |     |
| --- | ---- | --------- | ------- | --------- | ------------ | --- | --- | --- | ------------ | --- |
|     | 31   | Reserved  | MRO     | Reserved  |              |     |     |     |              | 0   |
Interrupt/exception nested overflow flag
bit, write 1 to clear:
0: Interrupt did not overflow;
1: Interrupt overflow flag.
Note: Interrupt overflow will only occur
|     |     |          |      | when  executing    |     | secondary     |              | interrupt  |     |     |
| --- | --- | -------- | ---- | ------------------ | --- | ------------- | ------------ | ---------- | --- | --- |
|     | 30  | NEST_OV  | MRW  |                    |     |               |              |            |     | 0   |
|     |     |          |      | service  function  |     | to  generate  | instruction  |            |     |     |
exception or NMI interrupt. At this time,
|     |     |     |     | the  exception  | and  | NMI  | interrupt  | enter  |     |     |
| --- | --- | --- | --- | --------------- | ---- | ---- | ---------- | ------ | --- | --- |
normally, but the CPU stack overflows, so
you cannot exit from this exception and
NMI interrupt.
| [29:12]  |     | Reserved  | MRO  | Reserved  |     |     |     |     |     | 0   |
| -------- | --- | --------- | ---- | --------- | --- | --- | --- | --- | --- | --- |
Nested status flag bit:
0000: No interrupt;
0001: Level 1 interrupt;
| [11:8]  |     | NEST_STA  | MRO  |     |     |     |     |     |     | 0   |
| ------- | --- | --------- | ---- | --- | --- | --- | --- | --- | --- | --- |
0011: level 2 interrupt (1-level nesting);
0111: Level 3 interrupt (overflow);
1111: Level 4 interrupt (overflow).
|       | [7:2]  | Reserved  | MRO  | Reserved  |     |     |     |     |     | 0   |
| ----- | ------ | --------- | ---- | --------- | --- | --- | --- | --- | --- | --- |
| V1.5  |        |           |      | 25        |     |     |     |     |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | ------------------- |
Nesting level:
00: Nesting is prohibited and the nesting
function is turned off;
01: First-level nesting, which turns on the
nesting function;
| [1:0]  | NEST_LVL  | MRW  |     |     |     | 0   |
| ------ | --------- | ---- | --- | --- | --- | --- |
Other: Invalid.
Note: Write 10 or 11 to this field, and the
field will be set to 01. When write 11 to this
field, read this register to get the highest
nesting level of the chip.

User mode global interrupt enable register (gintenr)
This register is invalid only for V3A:
| Name  | CSR Address  | Access  |     | Description  |     | Reset value  |
| ----- | ------------ | ------- | --- | ------------ | --- | ------------ |
gintenr  0x800  URW  Global interrupt enable register  0x00000000

This register is used to control the enable and mask of global interrupt. The enable and mask of global interrupt
in machine mode can be controlled by the MIE and MPIE bits in mstatus, but this register cannot be operated
in user mode. The global interrupt enable register gintenr is the mapping of MIE and MPIE in mstatus, and
can be used to set and clear MIE and MPIE by operating gintenr in user mode. Each of them is defined as:
| Bit  | Name  | Access  |                                   | Description  |                    | Reset value  |
| ---- | ----- | ------- | --------------------------------- | ------------ | ------------------ | ------------ |
|      |       |         | Used  to  mark                    | whether      | a  floating-point  |              |
| 31   | SD    | RO      | instruction is in a dirty state.  |              |                    | 0            |
Note: This bit applies to V3F only.
| [30:15]  | Reserved  | URO  | Reserved                  |              |                 | 0     |
| -------- | --------- | ---- | ------------------------- | ------------ | --------------- | ----- |
|          |           |      | Floating-point            | instruction  | set  state      | that  |
|          |           |      | allows  single-precision  |              | floating-point  |       |
instructions (F) to execute when the value
is not zero:
| [14:13]  | FS  | RO  | 00: OFF;  |     |     | 0   |
| -------- | --- | --- | --------- | --- | --- | --- |
01: Initial;
10: Clean;
11: Dirty.
Note: This bit applies to V3F only.
[12:11]  MPP  URO  Enter privileged mode before interruption.  0
| [10:8]  | Reserved  | URO  | Reserved  |     |     | 0   |
| ------- | --------- | ---- | --------- | --- | --- | --- |
When 0xBC0(CSR)bit5 is enabled, this bit
| 7   | MPIE  | URW  |     |     |     | 0   |
| --- | ----- | ---- | --- | --- | --- | --- |
can be read and written in user mode.
| [6:4]  | Reserved  | URO  | Reserved  |     |     | 0   |
| ------ | --------- | ---- | --------- | --- | --- | --- |
When 0xBC0(CSR)bit5 is enabled, this bit
| 3   | MIE  | URW  |     |     |     | 0   |
| --- | ---- | ---- | --- | --- | --- | --- |
can be read and written in user mode.
| [1:0]  | Reserved  | URO  | Reserved  |     |     | 0   |
| ------ | --------- | ---- | --------- | --- | --- | --- |

| V1.5  |     |     | 26  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
3.3 Interrupt Nesting
In conjunction with the interrupt configuration register PFIC_CFGR and the interrupt priority register
PFIC_IPRIOR, nesting of interrupts can be allowed to occur. Enable nesting in the interrupt configuration
register (Nesting is turned on by default for V3 series microprocessors) and configure the priority of the
corresponding interrupt. The smaller the priority value, the higher the priority. The smaller the value of the
preemption bit, the higher the preemption priority. If there are interrupts hanging at the same time under the
same preemption priority, the microprocessor responds to the interrupt with the lower priority value (higher
priority) first.

3.4 Hardware Prologue/Epilogue (HPE)
When an exception or interrupt occurs, the microprocessor stops the current program flow and shifts to the
execution of the exception or interrupt handling function, the site of the current program flow needs to be
saved. After the exception or interrupt returns, it is necessary to restore the site and continue the execution of
the stopped program flow. For V3 series microprocessors, the "site" here refers to all the Caller Saved registers
in Table 1-2.
The V3 series microprocessors support hardware single-cycle automatic saving of 16 of the integer Caller
Saved registers to an internal stack area that is not visible to the user. When an exception or interrupt returns,
the hardware single cycle automatically restores the data from the internal stack area to the 16 integer registers.
HPE supports nesting up to 2 levels deep.
A schematic of the microprocessor pressure stack is shown in the following figure.
Figure 3-1 Schematic diagram of pressure stack
Internal Hardware Stack
|     | X1     |        | X1  |     |
| --- | ------ | ------ | --- | --- |
|     | X5-7   | X5-7   |     |     |
|     | X10-11 | X10-11 |     |     |
X12-17
X12-17
|     | X28-31       | X28-31              |              |     |
| --- | ------------ | ------------------- | ------------ | --- |
|     | Single cycle | Single cycle        |              |     |
|     |  PUSH or POP |  PUSH or POP        |              |     |
|     | Ha r dw are  | Ha r dw are         | Har d w are  |     |
|     | P U SH       | Har d w are  P U SH | P O P        |     |
P O P
|     | IRQ1 | IRQ2 |     |     |
| --- | ---- | ---- | --- | --- |
Main
Priority Higher

Note: 1. Interrupt functions using the HPE need to be compiled using MRS or its provided toolchain and the
interrupt function needs to be declared with __attribute__((interrupt("WCH-Interrupt-fast"))).
2. The interrupt function using stack push is declared by __attribute__((interrupt())).

3.5 Vector Table Free (VTF)
The Programmable Fast Interrupt Controller (PFIC) provides 4 VTF channels, i.e., direct access to the interrupt
function entry without going through the interrupt vector table lookup process.
The VTF channel can be enabled by writing its interrupt number, interrupt service function base address and
| V1.5  |     |     | 27  |     |
| ----- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
offset address into the corresponding PFIC controller register while configuring an interrupt function normally.
The PFIC response process for fast and table-free interrupts is shown in Figure 3-2 below.

Figure 3-2 Schematic diagram of programmable fast interrupt controller
Peripherals
... Code
id_0 RISC-V CORE
BLE
|     |          |                     | (VTF: Vector table free) ... Handler_addr |     |
| --- | -------- | ------------------- | ----------------------------------------- | --- |
|     | ETH id_1 |      VTF(IRQn,addr) |                                           |     |
IRQ
id_2
USB
... FPIC ...
|     | ... |     | ... |     |
| --- | --- | --- | --- | --- |
id_n-1
|     | Other | Fast(id_num) | “j” |     |
| --- | ----- | ------------ | --- | --- |
id_n ...
EXC
Vector Table

|       |     |     |     |     |
| ----- | --- | --- | --- | --- |
| V1.5  |     |     | 28  |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
Chapter 4 Physical Memory Protection (PMP)
In order to improve the security of the system, the physical memory protection (PMP) module is designed
according to RISC-V architecture standard for the V3 series microprocessors of highland barley. Access rights
management of up to 4 physical region is supported. Permissions include read (R), write (W) and execute (X)
attributes, and the length of the protected area can be set to 4 bytes at least. PMP module always takes effect
in user mode, but it can take effect optionally by locking (L) attribute in machine mode.
If the access violates the current permission limit, it will trigger an abnormal interrupt. The PMP module
includes four groups of 8-bit configuration registers (One group of 32-bit) and four groups of address registers,
all of which need to be accessed in machine mode by CSR instruction.
Note: The number of protected areas supported by PMP in different models of microprocessors may be
different, and the number supported by pmpcfg and pmpaddr registers is also different. See Table 1-1 for
details.

4.1 PMP Register Set
The list of CSR registers supported by PMP module of V3 microprocessor is shown in Table 4-1 below.
Table 4-1 PMP module register set
|     | Name  | CSR address  |     | Access  |     | Description  |     | Reset value  |
| --- | ----- | ------------ | --- | ------- | --- | ------------ | --- | ------------ |
pmpcfg0  0x3A0  MRW  PMP configuration register 0  0x00000000
|     | pmpaddr0  | 0x3B0  |     | MRW  | PMP address register 0  |     |     | 0xXXXXXXXX  |
| --- | --------- | ------ | --- | ---- | ----------------------- | --- | --- | ----------- |
|     | pmpaddr1  | 0x3B1  |     | MRW  | PMP address register 1  |     |     | 0xXXXXXXXX  |
|     | pmpaddr2  | 0x3B2  |     | MRW  | PMP address register 2  |     |     | 0xXXXXXXXX  |
|     | pmpaddr3  | 0x3B3  |     | MRW  | PMP address register 3  |     |     | 0xXXXXXXXX  |

4.2 pmp<i>cfg Register
pmpcfg is the configuration register of PMP unit, and each register contains four 8-bit pmpcfg fields,
corresponding to the configuration of four regions, and pmpcfg represents the configuration value of region i.
Its format is shown in the following table 4-2.
Table 4-2 pmpcfg0 register
| 31  |          | 24  23  |          | 16  15   |          | 8  7  |          | 0        |
| --- | -------- | ------- | -------- | -------- | -------- | ----- | -------- | -------- |
|     | pmp3cfg  |         | pmp2cfg  |          | pmp1cfg  |       | pmp0cfg  | pmpcfg0  |

pmpcfg is used to configure area i, and its bit definition is described in the following table 4-3.
Table 4-3 pmp<i>cfg register
|     | Bit  Name  |     |     |     | Description  |     |     |     |
| --- | ---------- | --- | --- | --- | ------------ | --- | --- | --- |
Locking is enabled and can be unlocked in machine mode.
|     | 7   | L  0: Not locked;  |     |     |     |     |     |     |
| --- | --- | ------------------ | --- | --- | --- | --- | --- | --- |
1: Lock the relevant register.
|     | [6:5]  | -  Reserved  |     |     |     |     |     |     |
| --- | ------ | ------------ | --- | --- | --- | --- | --- | --- |
Address alignment and protection area range selection.
00: OFF (PMP off)
|     | [4:3]  | A  01: TOR (Top alignment protection)  |     |     |     |     |     |     |
| --- | ------ | -------------------------------------- | --- | --- | --- | --- | --- | --- |
10: NA4 (Fixed four-byte protection)
11: NAPOT (2(G+2) Byte protection, G≥1)
| V1.5  |     |     |     |     | 29  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- |

QingKeV3 Microprocessor Manual https://wch-ic.com
Executable attribute.
2 X 0: No execute permission;
1: Execute permission.
Writeable attribute.
1 W 0: No write permission
1: Write permission.
Readable attribute
0 R 0: No read permission
1: Read permission.
4.3 pmpaddr<i> Register
The pmpaddr register is used to configure the address of area I. The standard definition is under RV32
architecture, which is the encoding of the upper 32 bits of a 34-bit physical address, and its format is shown
in the following table 4-4. The whole physical address space of V3 microprocessor is 4G, so the upper two
bits of this register are not used.
Table 4-4 pmpaddr<i> register
31 0
address[33:2]
When NAPOT is selected, the low bit of the address register is also used to indicate the size of the current
protection area, as shown in the following table, where 'y' is a bit of the register.
Table 4-5 Relationship table between PMP configuration and address register and protected area
pmpaddr pmpcfg.A Matching base address and size
yyyy…yyyy NA4 With 'yy…yyyy00' as the base address, the 4-byte area is
protected.
yyyy…yyy0 NAPOT With 'yy…yyy000' as the base address, the 8-byte area is
protected.
yyyy…yy01 NAPOT With 'yy…yy0000' as the base address, the 16-byte area is
protected.
yyyy…y011 NAPOT With 'yy…y00000' as the base address, the 32-byte area is
protected.
… … …
yyy01…111 NAPOT With 'y0…000000' as the base address, the 231-byte area is
protected.
yy011…111 NAPOT Protect the entire 232-byte area.
4.4 Protection Mechanism
X/W/R in pmpcfg is used to set the protection authority of area I, and violation of relevant authority will cause
corresponding exception:
(1) When trying to fetch instructions in the PMP area without execution authority, it will cause an instruction
fetch access error exception (mcause=1).
(2) When trying to write data in the PMP area without write permission, it will cause an error exception
(mcause=7) in the store instruction access.
(3) When trying to read data in the PMP area without read permission, it will cause an abnormal memory
access error (mcause=5) for the load instruction.
A in pmpcfg is used to set the protection range and address alignment of region i, and to protect the memory
V1.5 30

QingKeV3 Microprocessor Manual https://wch-ic.com
of A_ADDR ≤ region < i > < B_ADDR (both A_ADDR and B_ADDR are required to be aligned in 4 bytes):
(1) If B _ ADDR–A_ADDR = = 22, NA4 mode is adopted;
(2) If B _ ADDR–A_ADDR = = 2(G+2), G≥1, and a _ addr is 2(G+2), the NAPOT method is adopted;
(3) Otherwise, the TOP mode is adopted.
Table 4-6 PMP address matching methods
A value Name Description
0b00 OFF No area to protect
Top Aligned Area Protection.
Under pmp<i>cfg, pmpaddr ≤ region<i> <pmpaddr；
i-1 i
pmpaddr = A_ADDR >> 2;
i-1
0b01 TOR
pmpaddr = B_ADDR >> 2.
i
Note: If area 0 of PMP is configured as TOR mode (i=0), the lower boundary of the
protection area is 0 address, i.e. 0 ≤ addr < pmpaddr0, all within the matching range.
Fixed 4-byte area protection.
0b10 NA4 pmp<i>cfg under pmpaddri as the starting address of the 4-byte pmpaddri =
A_ADDR>>2.
Protect the 2(G+2) region with G ≥ 1, when A_ADDR is 2(G+2) aligned.
0b11 NAPOT
pmpaddri = ((A_ADDR|(2(G+2)-1)) &~(1<<G+1))>>2.
The L bit in pmp<i>cfg is used to lock the PMP entry. After locking, the configuration register pmp<i>cfg and
the address register pmpaddr<i> will not be able to be modified. If A in pmp<i>cfg is set to TOR mode,
pmpaddr<i-1> will also not be modified. when L is set, the X/W/R permissions defined in pmp<i>cfg are also
valid for machine mode, and when L is cleared, X/W/R is only valid for user mode, and L is cleared only after
system reset.
QingKe V3 series microprocessors support protection of multiple zones. When the same operation matches
multiple zones at the same time, the zone with the smaller number is matched first.
V1.5 31

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |

Chapter 5 System Timer (SysTick)
QingKe V3 series microprocessor is designed with a 32-bit or 64-bit counter (SysTick) inside. Its clock source
is the system clock or its 8-frequency division, and V3A core of CH32V103 only supports 8-frequency division.
It can provide time base, timing and measuring time for real-time operating system. Different types of registers
involved in the timer have different mapping addresses, as shown in the following tables 5-1 and 5-2.
Table 5-1 V3A SysTick register list
|     | Name  | Access address  |     |     |     | Description  |     | Reset value  |     |
| --- | ----- | --------------- | --- | --- | --- | ------------ | --- | ------------ | --- |
STK_CTLR  0xE000F000  System counter control register  0x00000000
STK_CNTL  0xE000F004  System counter low register  0xXXXXXXXX
System counter high register
|     | STK_CNTH  | 0xE000F008  |     |     |     |     |     | 0xXXXXXXXX  |     |
| --- | --------- | ----------- | --- | --- | --- | --- | --- | ----------- | --- |
Note: Only valid for V3A.
STK_CMPLR  0xE000F00C  System count comparison value low register  0xXXXXXXXX
System count comparison value high register
|     | STK_CMPHR  | 0xE000F010  |     |     |     |     |     | 0xXXXXXXXX  |     |
| --- | ---------- | ----------- | --- | --- | --- | --- | --- | ----------- | --- |
Note: Only valid for V3A.
STK_SR  0xE000F014  System counter status register  0x00000000

Table 5-2 V3B/V3C/V3F/V3V SysTick register list
|     | Name  | Access address  |     |     |     | Description  |     | Reset value  |     |
| --- | ----- | --------------- | --- | --- | --- | ------------ | --- | ------------ | --- |
STK_CTLR  0xE000F000  System counter control register  0x00000000
STK_SR  0xE000F004  System counter status register  0x00000000
STK_CNTL  0xE000F008  Low register of system counter  0xXXXXXXXX
STK_CMPLR  0xE000F010  Count comparison value low register  0xXXXXXXXX

Each register is described in detail as follows.
System counter control register (STK_CTLR)
This register applies to V3A/B/C/F/V:
|     | Bit     | Name      | Access  |           |     | Description  |     | Reset value  |     |
| --- | ------- | --------- | ------- | --------- | --- | ------------ | --- | ------------ | --- |
|     | [31:7]  | Reserved  | RO      | Reserved  |     |              |     |              | 0   |
Counter assignment core register, which is used to
|     |     |     |     | configure  | the  | counter  | to  generate  | interrupt  |     |
| --- | --- | --- | --- | ---------- | ---- | -------- | ------------- | ---------- | --- |
requests to the specified core, and is automatically
|     | 6   | CID  | RW  |     |     |     |     |     | 0   |
| --- | --- | ---- | --- | --- | --- | --- | --- | --- | --- |
updated to the core ID of the operation when the
counter enable is configured to be valid.
Note: See specific chip for details.
Counter Initial Value Update:
1: Updates to 0 during upward counting; updates
to the comparison value during downward
|     | 5   | INIT  | RW1  |     |     |     |     |     | 0   |
| --- | --- | ----- | ---- | --- | --- | --- | --- | --- | --- |
counting.
0: Invalid.
Note: Does not apply to V3A.
Counting mode:
|     | 4   | MODE  | RW  |     |     |     |     |     | 0   |
| --- | --- | ----- | --- | --- | --- | --- | --- | --- | --- |
1: Count down;
| V1.5  |     |     |     |     | 32  |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
0: Count up.
Note: Invalid for V3A.
Automatic reload count enable bit:
1: Count from 0 again after counting up to the
comparison value, and count from the comparison
| 3   | STRE  | RW  |     | 0   |
| --- | ----- | --- | --- | --- |
value again after counting down to 0;
0: Continue counting up/down.
Note: Invalid for V3A.
Counter clock source selection bit:
| 2   | STCLK  | RW  1: HCLK as time base;  |     | 0   |
| --- | ------ | -------------------------- | --- | --- |
0: HCLK/8 as time base.
Counter interrupt enable control bits:
| 1   | STIE  | RW  1: Enable counter interrupt;  |     | 0   |
| --- | ----- | --------------------------------- | --- | --- |
0: Disable counter interrupt.
System counter enable control bit.
1: Enable system counter STK;
| 0   | STE  | RW  |     | 0   |
| --- | ---- | --- | --- | --- |
0: Disable the system counter STK and the counter
stops counting.

System counter status register (STK_SR)
This register applies to V3B/C/F/V:
| Bit  | Name  | Access  | Description  | Reset value  |
| ---- | ----- | ------- | ------------ | ------------ |
Software interrupt trigger enable (SW):
1: Trigger software interrupt;
| 31  | SWIE  | RW  0: Turn off the trigger.  |     | 0   |
| --- | ----- | ----------------------------- | --- | --- |
Note: This bit must be cleared after entering the
software interrupt, otherwise it will always trigger.
| [30:1]  | Reserved  | RO  Reserved  |     | 0   |
| ------- | --------- | ------------- | --- | --- |
Systick interrupt flags:
1: There is an unprocessed interrupt request for the
| 0   | STK_ISR0  | RW  |     | 0   |
| --- | --------- | --- | --- | --- |
timer;
0: There is no interrupt request for the timer.
Note: Multi-channel system counters may be implemented in different models of chips. For specific interrupt
flag width, please refer to chip manual.

This register applies to V3A:
| Bit     | Name      | Access        | Description  | Reset value  |
| ------- | --------- | ------------- | ------------ | ------------ |
| [31:2]  | Reserved  | RO  Reserved  |              | 0            |
Systick interrupt flag:
1: An unprocessed interrupt request exists for the
timer;
| 1   | STK_ISR0  | RW  |     | 0   |
| --- | --------- | --- | --- | --- |
0: No interrupt request exists for the timer.
Note: This bit is not applicable to the CH32V103
chip.
| 0     | SWIE  | RW  Software interrupt trigger enable (SWI):   |     | 0   |
| ----- | ----- | ---------------------------------------------- | --- | --- |
| V1.5  |       |                                                | 33  |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
1: Trigger software interrupt;
0: Turn off trigger.
Note: This bit must be cleared to 0 after entering
|     |     |     | the  software  |     | interrupt,  | otherwise  | it  | will  be  |
| --- | --- | --- | -------------- | --- | ----------- | ---------- | --- | --------- |
triggered all the time.

System counter low register (STK_CNTL)
This register applies to V3A/B/C/F/V:
| Bit  | Name  | Access  |     |     | Description  |     |     | Reset value  |
| ---- | ----- | ------- | --- | --- | ------------ | --- | --- | ------------ |
The current counter count value is 32 bits lower.
For V3A, this register can be read as 8-bit /16-bit
0xXXXXX
[31:0]  CNTL  RW  /32-bit, but can only be written as 8-bit, and other
XXX
models are not limited.
Note: CH32V103 only supports upward counting.
Note: Register STK_CNTL and register STK_CNTH in V3A together constitute a 64-bit system counter.

System counter high register (STK_CNTH)
This register applies to V3A:
| Bit  | Name  | Access  |     |     | Description  |     |     | Reset value  |
| ---- | ----- | ------- | --- | --- | ------------ | --- | --- | ------------ |
The current counter count value is 32 bits higher.
This register can be read by 8-bit/16-bit/32-bit, but
0xXXXXX
| [31:0]  | CNTH  | RW  | can only be written by 8-bit.  |     |     |     |     |     |
| ------- | ----- | --- | ------------------------------ | --- | --- | --- | --- | --- |
XXX
|     |     |     | Note:  | Only  | valid  for  V3A.  | CH32V103  |     | only  |
| --- | --- | --- | ------ | ----- | ----------------- | --------- | --- | ----- |
supports upward counting.
Note: Register STK_CNTL and register STK_CNTH in V3A together constitute a 64-bit system counter.

System count comparison value low register (STK_CMPLR)
This register applies to V3A/B/C/F/V:
| Bit  | Name  | Access  |     |     | Description  |     |     | Reset value  |
| ---- | ----- | ------- | --- | --- | ------------ | --- | --- | ------------ |
Set the counter comparison value 32 bits lower.
The STK interrupt will be triggered when the
count up CMP register value is equal to the CNT
|         |       |     | counter                 | value  | or  when  | the  count  | down  | CNT 0xXXXXX |
| ------- | ----- | --- | ----------------------- | ------ | --------- | ----------- | ----- | ----------- |
| [31:0]  | CMPL  | RW  |                         |        |           |             |       |             |
|         |       |     | counter value is zero.  |        |           |             |       | XXX         |
For V3A, this register can be read as 8-bit /16-bit
/32-bit, but can only be written as 8-bit, and other
models are not limited.
Note: The register STK_CMPLR and the register STK_CMPHR in V3A together constitute the 64-bit counter
comparison value.

System count comparison value high register (STK_CMPHR)
This register applies to V3A:
| Bit     | Name  | Access  |                                                   |     | Description  |     |     | Reset value  |
| ------- | ----- | ------- | ------------------------------------------------- | --- | ------------ | --- | --- | ------------ |
|         |       |         | Set the counter comparison value 32 bits higher.  |     |              |     |     | 0xXXXXX      |
| [31:0]  | CMPH  | RW      |                                                   |     |              |     |     |              |
|         |       |         | The STK interrupt will be triggered when the      |     |              |     |     | XXX          |
| V1.5    |       |         |                                                   | 34  |              |     |     |              |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
count up CMP register value is equal to the CNT
|     | counter  value  | or  when  | the  count  down  | CNT  |
| --- | --------------- | --------- | ----------------- | ---- |
counter value is zero.
This register can be read by 8-bit/16-bit/32-bit, but
can only be written by 8-bit.
Note: Only valid for V3A.
Note: The register STK_CMPLR and the register STK_CMPHR in V3A together constitute the 64-bit counter
comparison value.

| V1.5  | 35  |     |     |     |
| ----- | --- | --- | --- | --- |

QingKeV3 Microprocessor Manual https://wch-ic.com
Chapter 6 Processor Low-power Settings
QingKe V3 series microprocessors support sleep state via WFI (Wait For Interrupt) instruction to achieve low
static power consumption. Together with PFIC's system control register (PFIC_SCTLR), various Sleep modes
and WFE instructions can be implemented.
6.1 Enter Sleep
QingKe V3 series microprocessors can go to sleep in two ways, Wait for Interrupt (WFI) and Wait For Event
(WFE). The WFI method means that the microprocessor goes to sleep, waits for an interrupt to wake up, and
then wakes up to the corresponding interrupt to execute. The WFE method means that the microprocessor goes
to sleep, waits for an event to wake up, and wakes up to continue executing the previously stopped program
flow.
The standard RISC-V supports WFI instruction, and the WFI command can be executed directly to enter sleep
by WFI method. For the WFE method, the WFITOWFE bit in the system control register PFIC_SCTLR is
used to control the subsequent WFI commands as WFE processing to achieve the WFE method to enter sleep.
The depth of sleep is controlled according to the SLEEPDEEP bit in PFIC_SCTLR.
 If the SLEEPDEEP in PFIC_SCTLR register is cleared to zero, the microprocessor enters Sleep mode
and the internal unit clock is allowed to be turned off except for SysTick and part of the wake-up logic.
 If SLEEPDEEP in the PFIC_SCTLR register is set, the microprocessor enters Deep sleep mode and all
cell clocks are allowed to be turned off.
When the microprocessor is in Debug mode, it is not possible to enter any kind of Sleep mode.
6.2 Sleep Wakeup
QingKe V3 series microprocessors can be woken up after sleep due to WFI and WFE in the following ways.
 After the WFI method goes to sleep, it can be awakened by
(1) The microprocessor can be woken up by the interrupt source responded by the interrupt controller. After
waking up, the microprocessor executes the interrupt function first.
(2) Enter Sleep mode, debug request can make the microprocessor wake up and enter deep sleep, debug
request cannot wake up the microprocessor.
 After the WFE method goes to sleep, the microprocessor can be woken up by the following.
(1) Internal or external events, when there is no need to configure the interrupt controller, wake up and
continue to execute the program.
(2) If the SEVONPEND bit in PFIC_SCTLR is configured, the interrupt controller does not enable the
interrupt under, but when a new interrupt pending signal is generated (the previously generated pending
signal does not take effect), it can also make the microprocessor wake up, and the corresponding interrupt
pending flag needs to be cleared manually after waking up.
(3) Enter Sleep mode debug request can make the microprocessor wake up and enter deep sleep, debug request
cannot wake up the microprocessor.
In addition, the state of the microprocessor after wake-up can be controlled by configuring the SLEEPONEXIT
bit in PFIC_SCTLR.
V1.5 36

QingKeV3 Microprocessor Manual https://wch-ic.com
 SLEEPONEXIT is set and the last level interrupt return instruction (mret) will trigger the WFI mode
sleep.
 SLEEPONEXIT is cleared with no effect.
Various MCU products equipped with V3 series microprocessors can adopt different sleep modes, turn off
different peripherals and clocks, implement different power management policies and wake-up methods
according to different configurations of PFIC_SCTLR, and realize various low-power modes.
V1.5 37

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
Chapter 7 Debug Support
QingKe V3 series microprocessors include a hardware debug module that supports complex debugging
operations. When the microprocessor is suspended, the debug module can access the microprocessor's GPRs,
CSRs, Memory, external devices, etc. through abstract commands, program buffer deployment instructions,
etc. The debug module can suspend and resume the microprocessor's operation.
The debug module follows the RISC-V External Debug Support Version0.13.2 specification, detailed
documentation can be downloaded from RISC-V International website.

7.1 Debug Module
The debug module inside the microprocessor, capable of performing debug operations issued by the debug
host, includes.
  Access to registers through the debug interface
  Reset, suspend and resume the microprocessor through the debug interface
  Read and write memory, instruction registers and external devices through the debug interface
  Deploy multiple arbitrary instructions through the debug interface
  Set software breakpoints through the debug interface
  Set hardware breakpoints through the debug interface
  Support abstract command auto-execution
  Support single-step debugging
Note: V3A does not support hardware breakpoints, V3B hardware breakpoints support instruction address
matching, and V3C hardware breakpoints support instruction address and data address matching.

The internal registers of the debugging module use a 7-bit address code, and the following registers are
implemented inside QingKe V3 series microprocessors.
Table 7-1 Debug module register List
| Name  | Access address  |     | Description  |     |
| ----- | --------------- | --- | ------------ | --- |
data0  0x04  Data register 0, can be used for temporary storage of data
data1  0x05  Data register 1, can be used for temporary storage of data
| dmcontrol     | 0x10       | Debug module control register         |     |     |
| ------------- | ---------- | ------------------------------------- | --- | --- |
| dmstatus      | 0x11       | Debug module status register          |     |     |
| hartinfo      | 0x12       | Microprocessor status register        |     |     |
| abstractcs    | 0x16       | Abstract command status register      |     |     |
| command       | 0x17       | Abstract command register             |     |     |
| abstractauto  | 0x18       | Abstract command auto-execution       |     |     |
| progbuf0-7    | 0x20-0x27  | Instruction cache registers 0-7       |     |     |
| haltsum0      | 0x40       | Pause status register                 |     |     |
| capabi        | 0x7c       | Debugger interface register           |     |     |
| config        | 0x7d       | Debugger configuration register       |     |     |
| config_pre    | 0x7e       | Debugger pre-configuration registers  |     |     |

The debug host can control the microprocessor's suspend, resume, reset, etc. by configuring the dmcontrol
register. The RISC-V standard defines three types of abstract commands: access register, fast access, and
| V1.5  |     | 38  |     |     |
| ----- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
access memory. QingKe V3A microprocessor only supports register access, other models support register and
memory access, but not fast access. Access to registers (GPRs, CSRs) and continuous access to memory can
be realized by abstract commands.
The debug module implements 8 instruction cache registers progbuf0-7, and the debug host can cache multiple
instructions (which can be compressed instructions) to the buffer, and can choose to continue executing the
instructions in the instruction cache registers after executing the abstract command, or execute the cached
instructions directly. Note that the last instruction in the progbufs needs to be an "ebreak" or "c.ebreak"
instruction. Access to storage, peripherals, etc. is also possible through abstract commands and instructions
cached in the progbufs.

Each register is described in detail as follows.
Data register 0 (data0)
| Bit  | Name  | Access  | Description  | Reset Value  |
| ---- | ----- | ------- | ------------ | ------------ |
[31:0]  data0  RW  Data register 0, used for temporary storage of data  0

Data register 1 (data1)
| Bit  | Name  | Access  | Description  | Reset Value  |
| ---- | ----- | ------- | ------------ | ------------ |
[31:0]  data1  RW  Data register 1, used for temporary storage of data  0

Debug module control register (dmcontrol)
This register controls the pause, reset, and resume of the microprocessor. Debug host write data to the
corresponding field to achieve pause (haltreq), reset (ndmreset), resume (resumereq). You describe into the
following.
| Bit  | Name  | Access  | Description  | Reset Value  |
| ---- | ----- | ------- | ------------ | ------------ |
Pause the selected microprocessor, controlled by
hartsel.
| 31  | haltreq  | WO  |     | 0   |
| --- | -------- | --- | --- | --- |
0: Clear the pause request
1: Send a pause request
Restore the selected microprocessor, controlled by
hartsel.
0: Invalid
| 30  | resumereq  | W1  |     | 0   |
| --- | ---------- | --- | --- | --- |
1: Restore the current microprocessor
Note: Write 1 is valid and the hardware is cleared
after the microprocessor is recovered
| 29  | Reserved  | RO  Reserved  |     | 0   |
| --- | --------- | ------------- | --- | --- |
0: Invalid
28  ackhavereset  W1  1:  Clear  the  haverest  status  bit  of  the  0
microprocessor
| [27:18]  | Reserved  | RO  Reserved  |     | 0   |
| -------- | --------- | ------------- | --- | --- |
Microprocessor selection register, fixed value 0.
| [17:16]  | hartsel  | RW  |     | 0   |
| -------- | -------- | --- | --- | --- |
Note: Valid only for V3F.
| [15:2]  | Reserve  | R0  Reserve  |     | 0   |
| ------- | -------- | ------------ | --- | --- |
0: Clear reset
| 1   | ndmreset  | RW  |     | 0   |
| --- | --------- | --- | --- | --- |
1: Reset the entire system other than the debug
| V1.5  |     |     | 39  |     |
| ----- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | ------------------- |
module
0: Reset debug module
| 0   | dmactive  | RW  |     |     | 0   |
| --- | --------- | --- | --- | --- | --- |
1: Debug module works properly

Debug module status register (dmstatus)
This register is used to indicate the status of the debug module and is a read-only register with the following
description of each bit.
| Bit      | Name       | Access                                          | Description  |     | Reset Value  |
| -------- | ---------- | ----------------------------------------------- | ------------ | --- | ------------ |
| [31:23]  | Reserved   | RO  Reserved                                    |              |     | 0            |
|          |            | Fixed value, a value of 1 means that an ebreak  |              |     | V3A:0        |
| 22       | impebreak  | RO                                              |              |     |              |
|          |            | instruction is implied after progbuf.           |              |     | V3B/C/F:1    |
| [21:20]  | Reserved   | RO  Reserved                                    |              |     | 0            |
0: Invalid
| 19  | allhavereset  | RO  |     |     | 0   |
| --- | ------------- | --- | --- | --- | --- |
1: Microprocessor reset
0: Invalid
| 18  | anyhavereset  | RO  |     |     | 0   |
| --- | ------------- | --- | --- | --- | --- |
1: Microprocessor reset
0: Invalid
| 17  | allresumeack  | RO  |     |     | 0   |
| --- | ------------- | --- | --- | --- | --- |
1: Microprocessor reset
0: Invalid
| 16  | anyresumeack  | RO  |     |     | 0   |
| --- | ------------- | --- | --- | --- | --- |
1: Microprocessor reset
The selected microprocessor does not exist and is
controlled by hartsel
| 15  | allnoexit  | RO  0: Microprocessor does not exist   |     |     | 0   |
| --- | ---------- | -------------------------------------- | --- | --- | --- |
1: Microprocessor is optional
Note: Valid for V3F only.
The selected microprocessor does not exist and is
controlled by hartsel
| 14  | anynoexit  | RO  0: Microprocessor does not exist   |     |     | 0   |
| --- | ---------- | -------------------------------------- | --- | --- | --- |
1: Microprocessor is optional
Note: Valid for V3F only.
0: Invalid
| 13  | allavail  | RO  |     |     | 0   |
| --- | --------- | --- | --- | --- | --- |
1: Microprocessor is not available
0: Invalid
| 12  | anyavail  | RO  |     |     | 0   |
| --- | --------- | --- | --- | --- | --- |
1: Microprocessor is not available
0: Invalid
| 11  | allrunning  | RO  |     |     | 0   |
| --- | ----------- | --- | --- | --- | --- |
1: Microprocessor is running
0: Invalid
| 10  | anyrunning  | RO  |     |     | 0   |
| --- | ----------- | --- | --- | --- | --- |
1: Microprocessor is running
0: Invalid
| 9   | allhalted  | RO  |     |     | 0   |
| --- | ---------- | --- | --- | --- | --- |
1: Microprocessor is in suspension
0: Invalid
| 8   | anyhalted  | RO  |     |     | 0   |
| --- | ---------- | --- | --- | --- | --- |
1: Microprocessor out of suspension
|     |                | 0:  Authentication  | is  required  | before  using  | the  |
| --- | -------------- | ------------------- | ------------- | -------------- | ---- |
| 7   | authenticated  | RO  debug module    |               |                | 0x1  |
1: The debugging module has been certified
| V1.5  |     |     | 40  |     |     |
| ----- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |        |           |     |               |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | ------ | --------- | --- | ------------- | --- | --- | --- | --- | --- | ------------------- |
|                                 | [6:4]  | Reserved  |     | RO  Reserved  |     |     |     |     |     | 0                   |
Debugging system support architecture version
|     | [3:0]  | version  |     | RO  |     |     |     |     |     | 0x2  |
| --- | ------ | -------- | --- | --- | --- | --- | --- | --- | --- | ---- |
0010: V0.13

Microprocessor status register (hartinfo)
This register is used to provide information about the microprocessor to the debug host and is a read-only
register with each bit described as follows.
|     | Bit      | Name      | Access  |               |     | Description  |     |     |     | Reset Value  |
| --- | -------- | --------- | ------- | ------------- | --- | ------------ | --- | --- | --- | ------------ |
|     | [31:24]  | Reserved  |         | RO  Reserved  |     |              |     |     |     | 0            |
[23:20]  nscratch  RO  Number of dscratch registers supported  0x3
|     | [19:17]  | Reserved  |     | RO  Reserved  |     |     |     |     |     | 0   |
| --- | -------- | --------- | --- | ------------- | --- | --- | --- | --- | --- | --- |
0: Data register is mapped to CSR address
|     | 16  | dataaccess  |     | RO  |     |     |     |     |     | 0x1  |
| --- | --- | ----------- | --- | --- | --- | --- | --- | --- | --- | ---- |
1: Data register is mapped to memory address
|     | [15:12]  | datasize  |     | RO  Number of data registers  |                  |     |                |           |         | 0x2  |
| --- | -------- | --------- | --- | ----------------------------- | ---------------- | --- | -------------- | --------- | ------- | ---- |
|     |          |           |     | The                           | offset  address  |     | of  the  data  | register  | data0,  |      |
[11:0]  dataaddr  RO  whose base address is 0xe0000000, is subject to  0xXXX
specific reading.

Abstract command control and status registers (abstractcs)
This register is used to indicate the execution of the abstract command. The debug host can read this register
to know whether the last abstract command is executed or not, and can check whether an error is generated
during the execution of the abstract command and the type of the error, which is described in detail as follows.
|     | Bit      | Name      | Access  |           |     | Description  |     |     |     | Reset Value  |
| --- | -------- | --------- | ------- | --------- | --- | ------------ | --- | --- | --- | ------------ |
|     | [31:29]  | Reserved  | RO      | Reserved  |     |              |     |     |     | 0            |
Indicates the number of program buffer program
|     | [28:24]  | progbufsize  | RO  |     |     |     |     |     |     | 0x8  |
| --- | -------- | ------------ | --- | --- | --- | --- | --- | --- | --- | ---- |
cache registers
|     | [23:13]  | Reserved  | RO  | Reserved  |     |     |     |     |     | 0   |
| --- | -------- | --------- | --- | --------- | --- | --- | --- | --- | --- | --- |
0: No abstract command is executing
12  busy  RO  1: There are abstract commands being executed  0
Note: After execution, the hardware is cleared.
|     | 11  | Reserved  | RO  | Reserved  |     |     |     |     |     | 0   |
| --- | --- | --------- | --- | --------- | --- | --- | --- | --- | --- | --- |
Abstract command error type
000: No error
|     |     |     |     | 001:  Abstract  |     | command  | execution  | to  | write  | to  |
| --- | --- | --- | --- | --------------- | --- | -------- | ---------- | --- | ------ | --- |
command, abstractcs, abstractauto registers or read
and write to data and progbuf registers
010: Does not support current abstract command
[10:8]  cmder  RW  011: Execution of abstract command with exception  0
|     |     |     |     | 100:  The  | microprocessor  |     | is  not  | suspended  |     | or  |
| --- | --- | --- | --- | ---------- | --------------- | --- | -------- | ---------- | --- | --- |
unavailable and cannot execute abstract commands
101: Bus error
110: Parity bit error during communication
111: Other errors
Note: For bit writing 1 is used to clear the zero.
|       | [7:4]  | Reserved  | RO  | Reserved  |     |     |     |     |     | 0   |
| ----- | ------ | --------- | --- | --------- | --- | --- | --- | --- | --- | --- |
| V1.5  |        |           |     |           | 41  |     |     |     |     |     |

| QingKeV3 Microprocessor Manual  |            |     |                           |     |     |     | https://wch-ic.com  |
| ------------------------------- | ---------- | --- | ------------------------- | --- | --- | --- | ------------------- |
| [3:0]                           | datacount  | RO  | Number of data registers  |     |     |     | 0x2                 |

Abstract command register(command)
Debugging host can access GPRs, CSRs registers and memory by writing different configuration values into
the abstract command register.
When accessing the registers, the command register bits are defined as follows.
| Bit  | Name  | Access  |     |     | Description  |     | Reset Value  |
| ---- | ----- | ------- | --- | --- | ------------ | --- | ------------ |
Abstract command type
0: Access register;
| [31:24]  | cmdtype  | WO  |     |     |     |     | 0   |
| -------- | -------- | --- | --- | --- | --- | --- | --- |
1: Quick access (not supported);
2: Access to memory.
| 23  | Reserved  | WO  | Reserved  |     |     |     | 0   |
| --- | --------- | --- | --------- | --- | --- | --- | --- |
Access register data bit width
000: 8-bit
001: 16-bit
010: 32-bit
| [22:20]  | aarsize  | WO  |     |     |     |     | 0   |
| -------- | -------- | --- | --- | --- | --- | --- | --- |
011: 64-bit (not supported)
100: 128-bit (not supported)
|     |     |     | Note:  | When  | accessing  floating-point  | registers  |     |
| --- | --- | --- | ------ | ----- | -------------------------- | ---------- | --- |
FPRs, only 32-bit access is supported.
0: No effect
19  aarpostincrement  WO  1: Automatically increase the value of regno after  0
accessing the register
0: No effect
18  postexec  WO  1：Execute the abstract command and then execute  0
the command in progbuf
0: Do not execute the operation specified by write
| 17  | transfer  | WO  |     |     |     |     | 0   |
| --- | --------- | --- | --- | --- | --- | --- | --- |
1: Execute the manipulation specified by write
0: Copy data from the specified register to data0
16  write  WO  1: Copy data from data0 register to the specified  0
register
Specify access registers
0x0000-0x0fff are CSRs
| [15:0]  | regno  | WO  |     |     |     |     | 0   |
| ------- | ------ | --- | --- | --- | --- | --- | --- |
0x1000-0x101f are GPRs
0x1020-0x103f are FPRs

When accessing the memory of, the bits in the command register are defined as follows:
| Bit  | Name  | Access  |     |     | Description  |     | Reset Value  |
| ---- | ----- | ------- | --- | --- | ------------ | --- | ------------ |
Abstract command type
0: Access register;
| [31:24]  | cmdtype  | WO  |     |     |     |     | 0   |
| -------- | -------- | --- | --- | --- | --- | --- | --- |
1: Fast access (not supported);
2: Access memory.
0: Access physical address;
| 23  | aamvirtual  | WO  |     |     |     |     | 0   |
| --- | ----------- | --- | --- | --- | --- | --- | --- |
1: Access virtual address.
| [22:20]  | aamsize  | WO  | Access memory data bit width  |     |     |     | 0   |
| -------- | -------- | --- | ----------------------------- | --- | --- | --- | --- |
| V1.5     |          |     |                               | 42  |     |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
000: 8-bit;
001: 16-bit;
010: 32-bit;
011: 64-bit (not supported);
100: 128-bit (not supported);
0: No influence;
|     |     |     |     | 1:  After  | accessing  | the  | memory  | successfully,  |     |     |
| --- | --- | --- | --- | ---------- | ---------- | ---- | ------- | -------------- | --- | --- |
increase the address stored in the data1 register by
the number of bytes corresponding to the bit width
|     | 19  | aampostincrement  | WO  |     |     |     |     |     |     | 0   |
| --- | --- | ----------------- | --- | --- | --- | --- | --- | --- | --- | --- |
configured by aamsize.
Aamsize=0, accessed by byte, data1 plus 1.
Aamsize=1, accessed by half-word, data1 plus 2.
aamsize=2, accessed by bit, data1 plus 4.
0: No influence;
18  postexec  WO  1:  Execute  the  command  in  progbuf  after  0
executing the abstract command.
|     | 17  | Reserve  | RO  | Reserved  |     |     |     |     |     | 0   |
| --- | --- | -------- | --- | --------- | --- | --- | --- | --- | --- | --- |
0: Read data from the address specified by data1
to data0
|     | 16  | write  | WO  |     |     |     |     |     |     | 0   |
| --- | --- | ------ | --- | --- | --- | --- | --- | --- | --- | --- |
1: Write data in data0 to the address specified by
data1.
Definition of reading and writing mode
Write:
00, 01: Write directly to the memory;
10: After the data in data0 is OR with the data bits
|     |     |     |     | in  the  | memory,  | the  result  | is  | written  into  | the  |     |
| --- | --- | --- | --- | -------- | -------- | ------------ | --- | -------------- | ---- | --- |
[15:14]  target-specific  WO  memory (Only word access is supported).  0
11: After summing the data in data0 with the data
|     |     |     |     | bits  in  | the  memory,  | write  | the  | result  into  | the  |     |
| --- | --- | --- | --- | --------- | ------------- | ------ | ---- | ------------- | ---- | --- |
memory (Only word access is supported).
Read:
00, 01, 10, 11: Read 0 directly from the memory.
|     | [13:0]  | Reserve  | RO  | Reserved  |     |     |     |     |     | 0   |
| --- | ------- | -------- | --- | --------- | --- | --- | --- | --- | --- | --- |

Abstract command automatic execution register (abstractauto)
This register is used to configure the debugging module. When reading and writing progbufx and datax of the
debugging module, the abstract command can be executed again. The description of this register is as follows:
|     | Bit  | Name  | Access  |             |               | Description         |        |          | Reset Value  |     |
| --- | ---- | ----- | ------- | ----------- | ------------- | ------------------- | ------ | -------- | ------------ | --- |
|     |      |       |         | If  a  bit  | is  set,      | the  corresponding  |        | reading  | and          |     |
|     |      |       |         | writing     | of  progbufx  | will                | cause  | the      | abstract     |     |
command in the command register to be executed
|     | [31:16]  | autoexecprogbuf  | RW  |     |     |     |     |     |     | 0   |
| --- | -------- | ---------------- | --- | --- | --- | --- | --- | --- | --- | --- |
again.
|     |     |     |     | Note:  | V3  series  | is  designed  | with  | 8  progbufs,  |     |     |
| --- | --- | --- | --- | ------ | ----------- | ------------- | ----- | ------------- | --- | --- |
corresponding to bits [23:16].
|       | [15:12]  | Reserve  | RO  | Reserved  |     |     |     |     |     | 0   |
| ----- | -------- | -------- | --- | --------- | --- | --- | --- | --- | --- | --- |
| V1.5  |          |          |     |           | 43  |     |     |     |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
If a bit is set to 1, the corresponding reading and
writing of the datax register will cause the abstract
command in the Command register to be executed
|     | [11:0]  | autoexecdata  | RW  |     |     |     |     |     |     | 0   |
| --- | ------- | ------------- | --- | --- | --- | --- | --- | --- | --- | --- |
again.
|     |     |     |     | Note:  | V3  series  | is  designed  | with  | two  | data  |     |
| --- | --- | --- | --- | ------ | ----------- | ------------- | ----- | ---- | ----- | --- |
registers, corresponding to bits [1:0].

Instruction cache register (progbufx)
This register is used to store arbitrary instructions and deploy corresponding operations, including eight. The
V3A type core does not implicitly contain the "ebreak" instruction after progbuf, and the V3B/C/F type core
implicitly contains the "ebreak" instruction after progbuf.
|     | Bit  | Name  | Access  |     |     | Description  |     |     |     | Reset Value  |
| --- | ---- | ----- | ------- | --- | --- | ------------ | --- | --- | --- | ------------ |
Instruction encoding for cache operations, which
|     | [31:0]  | progbuf  | RW  |     |     |     |     |     |     | 0   |
| --- | ------- | -------- | --- | --- | --- | --- | --- | --- | --- | --- |
may include compression instructions

Pause status register (haltsum0)
This register is used to indicate whether the microprocessor is suspended or not. Each bit indicates the
suspended status of a microprocessor, and when there is only one core, only the lowest bit of this register is
used to indicate it.
|     | Bit     | Name      | Access  |           |     | Description  |     |     |     | Reset Value  |
| --- | ------- | --------- | ------- | --------- | --- | ------------ | --- | --- | --- | ------------ |
|     | [31:1]  | Reserved  | RO      | Reserved  |     |              |     |     |     | 0            |
haltsum0 V3F core suspend state
|     | 0   | haltsum0  | RO  | 0: Microprocessor operates normally   |     |     |     |     |     | 0   |
| --- | --- | --------- | --- | ------------------------------------- | --- | --- | --- | --- | --- | --- |
1: Microprocessor stop

Debug interface register (capabi)
This register is used to obtain information such as protocol and frequency supported by debugging.
|     | Bit  | Name  | Access  |     |     | Description  |     |     |     | Reset Value  |
| --- | ---- | ----- | ------- | --- | --- | ------------ | --- | --- | --- | ------------ |
[31:28]  swd_version  RO  1-wire interface version information  1
[27:24]  twd_version  RO  2-wire interface version information  1
|     |          |           |     | 1-wire  | interface  | sampling  | clock  | frequency,  |     |     |
| --- | -------- | --------- | --- | ------- | ---------- | --------- | ------ | ----------- | --- | --- |
|     | [23:18]  | SWD_FREQ  | RO  |         |            |           |        |             |     | 25  |
25MHz
|     |     |     |     | Debugger  |     | support  1/2-wire  | communication  |     |     |     |
| --- | --- | --- | --- | --------- | --- | ------------------ | -------------- | --- | --- | --- |
information
|     | [17:16]  | dmi_info  | RO  | 00: Dual line only   |     |     |     |     |     | 0x3  |
| --- | -------- | --------- | --- | -------------------- | --- | --- | --- | --- | --- | ---- |
01: Single line only
11: Single/dual line support
Flag bit for both single/two-wire communication
support
|     |     |          |     | 0:  Only  | single  | or  two-wire  | communication  |     | is  |     |
| --- | --- | -------- | --- | --------- | ------- | ------------- | -------------- | --- | --- | --- |
|     | 15  | SWD&TWD  | RO  |           |         |               |                |     |     | 1   |
supported
|     |     |     |     | 1:  Both  | single/two-wire  |     | communication  |     | is  |     |
| --- | --- | --- | --- | --------- | ---------------- | --- | -------------- | --- | --- | --- |
supported
|       | [14:11]  | Reserved    | RO  | Reserved                     |     |     |     |     |     | 0   |
| ----- | -------- | ----------- | --- | ---------------------------- | --- | --- | --- | --- | --- | --- |
|       | 10       | output_cap  | RO  | Debugger allows data output  |     |     |     |     |     | 1   |
| V1.5  |          |             |     |                              | 44  |     |     |     |     |     |

| QingKeV3 Microprocessor Manual  |        |           |     |           |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | ------ | --------- | --- | --------- | --- | --- | --- | --- | ------------------- | --- |
|                                 | [9:0]  | Reserved  | RO  | Reserved  |     |     |     |     |                     | 0   |

Debug interface configuration register (config)
This register is used to configure the debugger 1/2-wire mode, output enable, communication rate and other
information. This register does not support direct writing. It is necessary to write the value to the config_pre
register first, and then write the configuration password and configuration bit mask to this register. After this
register value is written and takes effect.
|     | Bit      | Name        | Access  |        |                | Description  |                      |     | Reset Value  |     |
| --- | -------- | ----------- | ------- | ------ | -------------- | ------------ | -------------------- | --- | ------------ | --- |
|     |          |             |         | Write  | configuration  |              | password  register,  |     | write        |     |
|     | [31:16]  | config_key  | WO      |        |                |              |                      |     |              | 0   |
operation takes effect when 0x5aa5 is written
1/2-wire selection register, write 1 to synchronize
swdmd_pre register value
|     | 15  | swdmd  | RW  |     |     |     |     |     |     | 1   |
| --- | --- | ------ | --- | --- | --- | --- | --- | --- | --- | --- |
1: 2-wire communication;
0: 1-wire communication.
|     | [14:11]  | Reserved  | RO  | Reserved  |     |     |     |     |     | 0   |
| --- | -------- | --------- | --- | --------- | --- | --- | --- | --- | --- | --- |
Debugger output enable, set 1 to allow output data,
write 1 to synchronize outen_pre register value
|     | 10  | outen  | RW  |     |     |     |     |     |     | 0   |
| --- | --- | ------ | --- | --- | --- | --- | --- | --- | --- | --- |
0: Output mask;
1: Output enable.
|     | [9:2]  | Reserved  | RO  | Reserved      |          |              |                |        |       | 0   |
| --- | ------ | --------- | --- | ------------- | -------- | ------------ | -------------- | ------ | ----- | --- |
|     |        |           |     | Synchronized  |          | outen_pre    | register       | value  | when  |     |
|     |        |           |     | writing       | 11b  to  | the  1-wire  | communication  |        | code  |     |
element rate selection register
[1:0]  psc1x  RW  00: 1-wire code element base frequency is 8M;  1
01: 1-wire code element base frequency is 4M;
10: 1-wire code element base frequency is 25M;
11: 1-wire code element base frequency is 12.5M.
Note: 10b and 11b of psc1x bits are unit symbol rates configured differently in different chips and are related
to capabi.SWD_FREQ values. See the specific chip manual for details.

Debug interface pre-configuration register (config_pre)
This register is used to pre-configure the debugger single/two-wire mode, output enable, communication rate
and other information. When the configuration password is written to this register, the write operation takes
effect. The config_pre register and config register need to be configured continuously, and other registers
cannot be operated during this period.
|     | Bit  | Name  | Access  |     |     | Description  |     |     | Reset Value  |     |
| --- | ---- | ----- | ------- | --- | --- | ------------ | --- | --- | ------------ | --- |
Write the configuration password register, write
0x5aa5 when the write operation takes effect, after
the configuration password is correct, read this
register to read 0x5aa5 that the pre-configuration
|     | [31:16]  | config_key  | WO  |     |     |     |     |     |     | 0   |
| --- | -------- | ----------- | --- | --- | --- | --- | --- | --- | --- | --- |
register is written successfully, allowing to write
|     |     |     |     | the  configuration  |     | register,  | otherwise  | read  | this  |     |
| --- | --- | --- | --- | ------------------- | --- | ---------- | ---------- | ----- | ----- | --- |
register value is 0, cannot configure the config
register.
|       | 15  | swdmd_pre  | RW  | 1/2-wire pre-select register  |     |     |     |     |     | 1   |
| ----- | --- | ---------- | --- | ----------------------------- | --- | --- | --- | --- | --- | --- |
| V1.5  |     |            |     |                               | 45  |     |     |     |     |     |

| QingKeV3 Microprocessor Manual  |            |                                 |     |     |     | https://wch-ic.com  |
| ------------------------------- | ---------- | ------------------------------- | --- | --- | --- | ------------------- |
| [14;11]                         | Reserved   | RO  Reserved                    |     |     |     | 0                   |
| 10                              | outen_pre  | RW  Debugger output pre-enable  |     |     |     | 0                   |
| [9:2]                           | Reserved   | RO  Reserved                    |     |     |     | 0                   |
1-wire communication symbol rate preselection
| [1:0]  | psc1x_pre  | RW  |     |     |     | 1   |
| ------ | ---------- | --- | --- | --- | --- | --- |
register

In addition to the above-mentioned registers of the debug module, the debug function also involves some CSR
registers, mainly the debug control and status register dcsr and the debug instruction pointer dpc, which are
described in detail as follows.

Debug control and status register (dcsr)
| Bit  | Name  | Access  | Description  |     |     | Reset Value  |
| ---- | ----- | ------- | ------------ | --- | --- | ------------ |
0000: External debugging is not supported
0100: Support standard external debugging
| [31:28]  | xdebugver  | DRO  |     |     |     | 0x4  |
| -------- | ---------- | ---- | --- | --- | --- | ---- |
1111: External debugging is supported, but does not meet
the specification
| [27:16]  | Reserved  | DRO  Reserved  |     |     |     | 0   |
| -------- | --------- | -------------- | --- | --- | --- | --- |
0: The ebreak command in machine mode behaves as
described in the privilege file
| 15  | ebreakm  | DRW  |     |     |     | 0   |
| --- | -------- | ---- | --- | --- | --- | --- |
1: The ebreak command in machine mode can enter debug
mode
| [14:13]  | Reserved  | DRO  Reserved    |          |                 |          | 0   |
| -------- | --------- | ---------------- | -------- | --------------- | -------- | --- |
|          |           | 0:  The  ebreak  | command  | in  user  mode  | behaves  | as  |
described in the privilege file
| 12  | ebreaku  | DRW  |     |     |     | 0   |
| --- | -------- | ---- | --- | --- | --- | --- |
1: The ebreak command in user mode can enter debug
mode
0: Interrupts are disabled under single-step debugging
| 11  | stepie  | DRW  |     |     |     | 0   |
| --- | ------- | ---- | --- | --- | --- | --- |
1: Enable interrupts under single-step debugging
| 10  | Reserved  | DRO  Reserved  |     |     |     | 0   |
| --- | --------- | -------------- | --- | --- | --- | --- |
0: System timer running in Debug mode
| 9   | stoptime  | DRW  |     |     |     | 0   |
| --- | --------- | ---- | --- | --- | --- | --- |
1: System timer stop in Debug mode
Reasons for entering debugging
001: Entering debugging in the form of ebreak command
(priority 3)
010: Entering debugging in the form of trigger module
(priority 4, the highest)
011: Entering debugging in the form of pause request
| [8:6]  | cause  | DRO  |     |     |     | 0   |
| ------ | ------ | ---- | --- | --- | --- | --- |
(priority 1)
100: debugging in the form of single-step debugging
(priority 0, the lowest)
101: enter debug mode directly after microprocessor reset
(priority 2)
Others: Reserved
| [5:4]  | Reserved  | DRO  Reserved                           |     |     |     | 0   |
| ------ | --------- | --------------------------------------- | --- | --- | --- | --- |
| 2      | step      | DRW  0: Turn off single-step debugging  |     |     |     | 0   |
| V1.5   |           |                                         | 46  |     |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
1: Enable single-step debugging
Non-maskable interrupt pending status
| 3   | nmi_pend  | DRO  |     | 0   |
| --- | --------- | ---- | --- | --- |
Note: Valid only for V3F.
Privilege mode
00: User mode
01: Supervisor mode (not supported)
10: Reserved
| [1:0]  | prv  | DRW  |     | 0   |
| ------ | ---- | ---- | --- | --- |
11: Machine mode
Note: Record the privileged mode when entering debug
mode, the debugger can modify this value to modify the
privileged mode when exiting debug

Debug mode program pointer (dpc)
This register is used to store the address of the next instruction to be executed after the microprocessor enters
debug mode, and its value is updated with different rules depending on the reason for entering debug. dpc
register is described in detail as follows.
| Bit     | Name  | Access                    | Description  | Reset Value  |
| ------- | ----- | ------------------------- | ------------ | ------------ |
| [31:0]  | dpc   | DRW  Instruction address  |              | 0            |

The rules for updating the registers are shown in the following table.
| Enter the debugging method  |         |                                    | dpc Update rules  |     |
| --------------------------- | ------- | ---------------------------------- | ----------------- | --- |
|                             | ebreak  | Address of the Ebreak instruction  |                   |     |
single step  Instruction address of the next instruction of the current instruction
| trigger module  |     | Temporarily not supported  |     |     |
| --------------- | --- | -------------------------- | --- | --- |
halt request  Address of the next instruction to be executed when entering Debug

7.2 Debug Interface
Different from the standard JTAG interface defined by RISC-V, QingKe V3 series microprocessor adopts 1-
wire/2-wire serial debug interface and follows WCH debug interface protocol V1.0. The debug interface is
responsible for the communication between the debug host and the debug module, and realizes the read/write
operation of the debug host to the debug module registers. WCH designed WCH_Link and open source its
schematic and program binary files, which can be used for debugging all microprocessors of RISC-V
architecture.
Refer to WCH Debug Protocol Manual for specific debug interface protocols.
|       |     |     |     |     |
| ----- | --- | --- | --- | --- |
| V1.5  |     |     | 47  |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
Chapter 8 CSR Register List
The RISC-V architecture defines a number of Control and Status Registers (CSRs) for controlling and
recording the operating status of the microprocessor. Some of the CSRs have been introduced in the previous
section, and this chapter will detail the CSR registers implemented in the QingKe V3 series microprocessors.

8.1 CSR Register List
Table 8-1 List of Microprocessor CSR Registers
| Type  |            | Name     | CSR Address  |        | Access  |     |     |                                | Description  |     |     |     |
| ----- | ---------- | -------- | ------------ | ------ | ------- | --- | --- | ------------------------------ | ------------ | --- | --- | --- |
|       | mvendorid  |          |              | 0xF11  | MRO     |     |     | Chip Manufacturer ID Register  |              |     |     |     |
|       |            | marchid  |              | 0xF12  | MRO     |     |     | Architecture number register   |              |     |     |     |
mimpid  0xF13  MRO  Hardware implementation numbering register
|         |             | mhartid   |          | 0xF14  | MRO  |     |     |                                     | Core ID register      |     |     |     |
| ------- | ----------- | --------- | -------- | ------ | ---- | --- | --- | ----------------------------------- | --------------------- | --- | --- | --- |
|         |             | mstatus   |          | 0x300  | MRW  |     |     |                                     | Status register       |     |     |     |
|         |             | misa      |          | 0x301  | MRW  |     |     | Hardware instruction set register   |                       |     |     |     |
|         |             | mtvec     |          | 0x305  | MRW  |     |     | Exception base address register     |                       |     |     |     |
|         |             | mscratch  |          | 0x340  | MRW  |     |     | Machine mode staging register       |                       |     |     |     |
|         |             | mepc      |          | 0x341  | MRW  |     |     | Exception program pointer register  |                       |     |     |     |
|         |             | mcause    |          | 0x342  | MRW  |     |     | Exception cause register            |                       |     |     |     |
|         |             | mtval     |          | 0x343  | MRW  |     |     | Exception value register            |                       |     |     |     |
|         | pmpcfg<i>   |           | 0x3A0+i  |        | MRW  |     |     | PMP configuration register          |                       |     |     |     |
| RISC-V  | pmpaddr<i>  |           | 0x3B0+i  |        | MRW  |     |     |                                     | PMP address register  |     |     |     |
Standard CSR  tselect  0x7A0  MRW  Debug trigger selection register
|     |            | tdata1  | 0x7A1  |     | MRW  |     |                                      | Debug trigger data register 1       |                   |     |     |     |
| --- | ---------- | ------- | ------ | --- | ---- | --- | ------------------------------------ | ----------------------------------- | ----------------- | --- | --- | --- |
|     |            | tdata2  | 0x7A2  |     | MRW  |     |                                      | Debug trigger data register 2       |                   |     |     |     |
|     |            | tinfo   | 0x7A4  |     | MRO  |     |                                      | Trigger information register        |                   |     |     |     |
|     |            | dcsr    | 0x7B0  |     | DRW  |     |                                      | Debug control and status registers  |                   |     |     |     |
|     |            | dpc     | 0x7B1  |     | DRW  |     | Debug mode program pointer register  |                                     |                   |     |     |     |
|     | dscratch0  |         | 0x7B2  |     | DRW  |     |                                      | Debug mode staging register 0       |                   |     |     |     |
|     | dscratch1  |         | 0x7B3  |     | DRW  |     |                                      | Debug mode staging register 1       |                   |     |     |     |
|     | dbgmcu_0   |         | 0x7C0  |     | MRW  |     |                                      |                                     | Debug Register 0  |     |     |     |
Floating point accumulation exception flag
|     |     | fflags  |     | 0x001  | RW  |     |     |     |     |     |     |     |
| --- | --- | ------- | --- | ------ | --- | --- | --- | --- | --- | --- | --- | --- |
register
|     |     | frm      |     | 0x002  | RW   |     | Floating point rounding mode register       |                                   |     |     |     |     |
| --- | --- | -------- | --- | ------ | ---- | --- | ------------------------------------------- | --------------------------------- | --- | --- | --- | --- |
|     |     | fcsr     |     | 0x003  | RW   |     | Floating point control and status register  |                                   |     |     |     |     |
|     |     | gintenr  |     | 0x800  | URW  |     |                                             | Global interrupt enable register  |     |     |     |     |
Vendor  intsyscr  0x804  URW  Interrupt system control register
defined CSR
|     |     | corecfgr  | 0xBC0  |     | MRW  |     | Microprocessor configuration register  |                                    |     |     |     |     |
| --- | --- | --------- | ------ | --- | ---- | --- | -------------------------------------- | ---------------------------------- | --- | --- | --- | --- |
|     |     | inestcr   | 0xBC1  |     | MRW  |     |                                        | Interrupt nested control register  |     |     |     |     |

Table 8-2 List of RISC-V3V Microprocessor CSR Registers
| Type  |                 | Name   |     | CSR Address  |     | Access  |     |                                         |                                | Description  |     |     |
| ----- | --------------- | ------ | --- | ------------ | --- | ------- | --- | --------------------------------------- | ------------------------------ | ------------ | --- | --- |
|       |                 | vxsat  |     | 0x009        |     | URW     |     | Fixed-point saturation status register  |                                |              |     |     |
|       |                 | vxrm   |     | 0x00A        |     | URW     |     |                                         | Fixed-point rounding register  |              |     |     |
|       |                 | vcsr   |     | 0x00F        |     | URW     |     | Vector control and status register      |                                |              |     |     |
|       | mcount_inhibit  |        |     | 0x320        |     | MRW     |     |                                         | Counter mask register          |              |     |     |
RISC-V3V
|     |     | mcycle  |     | 0xB00  |     | MRW  |     |     | Machine cycle counter register  |     |     |     |
| --- | --- | ------- | --- | ------ | --- | ---- | --- | --- | ------------------------------- | --- | --- | --- |
CSR
|     |     | minstret  |     | 0xB02  |     | MRW  |     | Machine instruction counter register  |       |              |     |          |
| --- | --- | --------- | --- | ------ | --- | ---- | --- | ------------------------------------- | ----- | ------------ | --- | -------- |
|     |     | ucycle    |     | 0xC00  |     | RO   |     | User mode cycle counter register      |       |              |     |          |
|     |     |           |     |        |     |      |     | User                                  | mode  | instruction  |     | counter  |
|     |     | uinstret  |     | 0xC02  |     | RO   |     |                                       |       |              |     |          |
register
| V1.5  |     |     |     |     |     | 48  |     |     |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |           |        |     |      |                                       | https://wch-ic.com  |
| ------------------------------- | --------- | ------ | --- | ---- | ------------------------------------- | ------------------- |
|                                 | vl        | 0xC20  |     | URO  | Element body length register          |                     |
|                                 | vtype     | 0xC21  |     | URO  | Vector data type register             |                     |
|                                 | vlenb     | 0xC22  |     | URO  | Vector register byte length register  |                     |
|                                 | vcontrol  | 0x805  |     | URW  | Custom vector control register        |                     |
Custom vector register stack base
|     | vppaddr  | 0x806  |     | URW  |     |     |
| --- | -------- | ------ | --- | ---- | --- | --- |
address register
Custom vector exception source
|     | vcause  | 0x808  |     | URO  |     |     |
| --- | ------- | ------ | --- | ---- | --- | --- |
register
Custom vector exception trap
|     | vtval  | 0x809  |     | URO  |     |     |
| --- | ------ | ------ | --- | ---- | --- | --- |
information register

8.2 RISC-V Standard CSR Registers
Chip Manufacturer ID Register (marchid)
| Bit  | Name  | Access  |     | Description  |     | Reset Value  |
| ---- | ----- | ------- | --- | ------------ | --- | ------------ |
Chip Manufacturer ID, fixed value 0.
| [31:0]  | mvendorid  | MRO  |     |     |     | 0   |
| ------- | ---------- | ---- | --- | --- | --- | --- |
Note: Valid for V3F only.

Architecture number register (marchid)
This register is a read-only register to indicate the current microprocessor hardware architecture number, which
is mainly composed of vendor code, architecture code, series code, and version code. Each of them is defined
as follows.
| Bit  | Name      | Access         |     | Description  |     | Reset Value  |
| ---- | --------- | -------------- | --- | ------------ | --- | ------------ |
| 31   | Reserved  | MRO  Reserved  |     |              |     | 1            |
Manufacturer code 0
| [30:26]  | Vender0  | MRO  |     |     |     | 0x17  |
| -------- | -------- | ---- | --- | --- | --- | ----- |
Fixed to the letter "W" code
Manufacturer code1
| [25:21]  | Vender1  | MRO  |     |     |     | 0x03  |
| -------- | -------- | ---- | --- | --- | --- | ----- |
Fixed to the letter "C" code
Manufacturer code 2
| [20:16]  | Vender2  | MRO  |     |     |     | 0x08  |
| -------- | -------- | ---- | --- | --- | --- | ----- |
Fixed to the letter "H" code
| 15  | Reserved  | MRO  Reserved  |     |     |     | 1   |
| --- | --------- | -------------- | --- | --- | --- | --- |
Architecture code
| [14:10]  | Arch  | MRO  |     |     |     | 0x16  |
| -------- | ----- | ---- | --- | --- | --- | ----- |
RISC-V architecture is fixed to the letter "V" code
Series code
| [9:5]  | Serial  | MRO  |     |     |     | 0x03  |
| ------ | ------- | ---- | --- | --- | --- | ----- |
QingKe V3 series, fixed to the number "3"
Version code
[4:0]  Verision  MRO  Can be the version "A", "B", "C" and other letters  x
of the code
The manufacturer number and version number are alphabetic, and the series number is numeric. The coding
table of letters is shown in the following table.
Table 8-1 Alphabetic Mapping Table
A  B  C  D  E  F  G  H  I  J  K  L  M  N  O  P  Q  R  S  T  U  V  W  X  Y  Z
1  2  3  4  5  6  7  8  9  10  11  12  13  14  15  16  17  18  19  20  21  22  23  24  25  26
Among them, QingKe V3A microprocessor, the register reads back to 0.
| V1.5  |     |     | 49  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | ------------------- | --- |

Hardware implementation numbering register (mimpid)
This register is mainly composed of vendor codes, each of which is defined as follows.
| Bit  | Name      | Access         | Description  |     |     | Reset Value  |
| ---- | --------- | -------------- | ------------ | --- | --- | ------------ |
| 31   | Reserved  | MRO  Reserved  |              |     |     | 1            |
Manufacturer code 0
| [30:26]  | Vender0  | MRO  |     |     |     | 0x17  |
| -------- | -------- | ---- | --- | --- | --- | ----- |
Fixed to the letter "W" code
Manufacturer code1
| [25:21]  | Vender1  | MRO  |     |     |     | 0x03  |
| -------- | -------- | ---- | --- | --- | --- | ----- |
Fixed to the letter "C" code
Manufacturer code 2
| [20:16]  | Vender2  | MRO  |     |     |     | 0x08  |
| -------- | -------- | ---- | --- | --- | --- | ----- |
Fixed to the letter "H" code
| 15      | Reserved  | MRO  Reserved           |     |     |     | 1    |
| ------- | --------- | ----------------------- | --- | --- | --- | ---- |
| [14:8]  | Reserved  | MRO  Reserved           |     |     |     | 0    |
| [7:4]   | Minor     | MRO  Subversion number  |     |     |     | 0xX  |
This register is readable in any machine implementation, and in the QingKe V3A series processor, this register
reads back to zero.

Core ID register (mhartid)
| Bit  | Name  | Access  | Description  |     |     | Reset Value  |
| ---- | ----- | ------- | ------------ | --- | --- | ------------ |
Core ID, indicating the core ID number of this
| [31:0]  | mhartid  | MRO  core in the chip architecture.  |     |     |     | 0   |
| ------- | -------- | ------------------------------------ | --- | --- | --- | --- |
Note: Valid only for V3F.

Machine mode status register (mstatus)
This register has been partially described in the previous section, and its folks are positioned as follows.
| Bit  | Name  | Access  | Description  |     |     | Reset Value  |
| ---- | ----- | ------- | ------------ | --- | --- | ------------ |
Used to mark whether a floating-point instruction
| 31  | SD  | MRO  is in a dirty state.  |     |     |     | 0   |
| --- | --- | -------------------------- | --- | --- | --- | --- |
Note: Valid only for V3F.
| [30:13]  | Reserved  | MRO  Reserved   |              |                   |         | 0   |
| -------- | --------- | --------------- | ------------ | ----------------- | ------- | --- |
|          |           | Floating-point  | instruction  | set  state  that  | allows  |     |
single-precision floating-point instructions(F) to
execute when the value is not zero:
00: OFF;
| [14:13]  | FS  | MRW  |     |     |     | 0   |
| -------- | --- | ---- | --- | --- | --- | --- |
01: Initial;
10: Clean;
11: Dirty.
Note: Valid only for V3F.
| [12:11]  | MPP  | MRW  Privileged mode before entering break  |     |     |     | 0   |
| -------- | ---- | ------------------------------------------- | --- | --- | --- | --- |
Vector instruction set state, value other than 0
allows vector instruction (V) execution:
00: OFF;
| [10:9]  | VS  | MRW  |     |     |     | 0   |
| ------- | --- | ---- | --- | --- | --- | --- |
01: Initial;
10: Clean;
11: Dirty.
| V1.5  |     |     | 50  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |           |                |     | https://wch-ic.com  |
| ------------------------------- | --------- | -------------- | --- | ------------------- |
| 8                               | Reserved  | MRO  Reserved  |     | 0                   |
7  MPIE  MRW  Interrupt enable state before entering interrupt  0
| [6:4]  | Reserved  | MRO  Reserved                       |     | 0   |
| ------ | --------- | ----------------------------------- | --- | --- |
| 3      | MIE       | MRW  Machine mode interrupt enable  |     | 0   |
| [2:0]  | Reserved  | MRO  Reserved                       |     | 0   |

The MPP field is used to save the privileged mode before entering the exception or interrupt, and is used to
restore the privileged mode after exiting the exception or interrupt. MIE is the global interrupt enable bit, and
when entering the exception or interrupt, the value of MPIE is updated to the value of MIE, and it should be
noted that in the QingKe V3 series microprocessors, MIE will not be updated to 0 before the last level of
nested interrupts to ensure that the interrupt nesting in Machine mode continues to be executed. When an
exception or interrupt is exited, the microprocessor reverts to the Machine mode saved by MPP and the MIE
is restored to the MPIE value.
QingKe V3 microprocessor supports Machine mode and User mode, if you need to make the microprocessor
only work in Machine mode, you can set the MPP to 0x3 in the initialization of the boot file, that is, after
returning, it will always remain in Machine mode.

Hardware instruction set register (misa)
This register is used to indicate the architecture of the microprocessor and the supported instruction set
extensions, each of which is described as follows.
| Bit  | Name  | Access  | Description  | Reset Value  |
| ---- | ----- | ------- | ------------ | ------------ |
Machine word length
01:32
| [31:30]  | MXL  | MRO  |     | 0x1  |
| -------- | ---- | ---- | --- | ---- |
10:64
11:128
| [29:26]  | Reserved    | MRO  Reserved                    |     | 0   |
| -------- | ----------- | -------------------------------- | --- | --- |
| [25:0]   | Extensions  | MRO  Instruction set extensions  |     | x   |

The MXL is used to indicate the word length of the microprocessor, QingKe V3 are 32-bit microprocessors,
the domain is fixed to 1. Extensions are used to indicate that the microprocessor supports extended instruction
set details, each indicates a class of extensions, its detailed description is shown in the following table.
Table 8-2 Instruction Set Extension Details
| Bit  | Name  |                                                      | Description  |     |
| ---- | ----- | ---------------------------------------------------- | ------------ | --- |
| 0    | A     | Atomic extension                                     |              |     |
| 1    | B     | Tentatively reserved for Bit-Manipulation extension  |              |     |
| 2    | C     | Compressed extension                                 |              |     |
| 3    | D     | Double-precision floating-point extension            |              |     |
| 4    | E     | RV32E base ISA                                       |              |     |
| 5    | F     | Single-precision floating-point extension            |              |     |
| 6    | G     | Additional standard extensions present               |              |     |
| 7    | H     | Hypervisor extension                                 |              |     |
| 8    | I     | RV32I/64I/128I base ISA                              |              |     |
9  J  Tentatively reserved for Dynamically Translated Languages extension
| 10    | K   | Reserved  |     |     |
| ----- | --- | --------- | --- | --- |
| V1.5  |     |           | 51  |     |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
11  L  Tentatively reserved for Decimal Floating-Point extension
| 12  | M   | Integer Multiply/Divide extension               |     |     |
| --- | --- | ----------------------------------------------- | --- | --- |
| 13  | N   | User-level interrupts supported                 |     |     |
| 14  | O   | Reserved                                        |     |     |
| 15  | P   | Tentatively reserved for Packed-SIMD extension  |     |     |
| 16  | Q   | Quad-precision floating-point extension         |     |     |
| 17  | R   | Reserved                                        |     |     |
| 18  | S   | Supervisor mode implemented                     |     |     |
19  T  Tentatively reserved for Transactional Memory extension
| 20  | U   | User mode implemented                      |     |     |
| --- | --- | ------------------------------------------ | --- | --- |
| 21  | V   | Tentatively reserved for Vector extension  |     |     |
| 22  | W   | Reserved                                   |     |     |
| 23  | X   | Non-standard extensions present            |     |     |
| 24  | Y   | Reserved                                   |     |     |
| 25  | Z   | Reserved                                   |     |     |
For example, for QingKe V3A microprocessor, the register value is 0x401001105, which means that the
supported instruction set architecture is RV32IMAC, and it has User mode implementation.

Machine mode exception base address register (mtvec)
This register is used to store the base address of the exception or interrupt handler and the lower two bits are
used to configure the mode and identification method of the vector table as described in Section 3.2.

Machine mode staging register (mscratch)
| Bit     | Name      | Access             | Description  | Reset Value  |
| ------- | --------- | ------------------ | ------------ | ------------ |
| [31:0]  | mscratch  | MRW  Data storage  |              | 0            |
This register is a 32-bit readable and writable register in machine mode for temporary data storage. For
example, when entering an exception or interrupt handler, the user stack pointer SP is stored in this register
and the interrupt stack pointer is assigned to the SP register. After exiting the exception or interrupt, restore
the value of user stack pointer SP from mscratch. That is, the interrupt stack and user stack can be isolated.

Machine mode exception program pointer register (mepc)
| Bit     | Name  | Access                            | Description  | Reset Value  |
| ------- | ----- | --------------------------------- | ------------ | ------------ |
| [31:0]  | mepc  | MRW  Exception procedure pointer  |              | 0            |
This register is used to save the program pointer when entering an exception or interrupt. It is used to save the
instruction PC pointer before entering an exception when an exception or interrupt is generated, and mepc is
used as the return address when the exception or interrupt is handled and used for exception or interrupt return.
However, it is important to note that.
  When an exception occurs, mepc is updated to the PC value of the instruction currently generating the
exception.
  When an interrupt occurs, mepc is updated to the PC value of the next instruction.
When you need to return an exception after processing the exception, you should pay attention to modifying
the value of the mepc, and more details can be found in Chapter 2 Exceptions.

Machine mode exception cause register (mcause)
| Bit   | Name  | Access  | Description  | Reset Value  |
| ----- | ----- | ------- | ------------ | ------------ |
| V1.5  |       |         | 52           |              |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | ------------------- |
Interrupt indication field
| 31  | Interrupt  | MRW  0: Exception   |     |     |     | 0   |
| --- | ---------- | ------------------- | --- | --- | --- | --- |
1: Interruption
[30:0]  Exception Code  MRW  Exception codes, see Table 2-1 for details  0
This register is mainly used to store the cause of the exception or the interrupt number of the interrupt. Its
highest bit is the Interrupt field, which is used to indicate whether the current occurrence is an exception or an
interrupt. The lower bit is the exception code, which is used to indicate the specific cause. Its details can be
found in Chapter 2 Exceptions.

Machine mode exception value register (mtval)
| Bit     | Name   | Access                |     | Description  |     | Reset Value  |
| ------- | ------ | --------------------- | --- | ------------ | --- | ------------ |
| [31:0]  | mtval  | MRW  Exception value  |     |              |     | 0            |
This register is used to hold the value that caused the exception when an exception occurs. For details such as
the value and time of its storage, please refer to Chapter 2 Exceptions.

PMP configuration register (pmpcfg<i>)
This register is mainly used to configure the physical memory protection unit, and every 8 bits of this register
are used to configure the protection of an area. Please refer to Chapter 4 for the detailed definition.

PMP address register (pmpaddr<i>)
This register is mainly used for the address configuration of the physical memory protection unit, which
encodes the upper 32 bits of a 34-bit physical address. Please refer to Chapter 4 for the specific configuration
method.

Debug trigger select register (tselect)
It is only valid for microprocessors that support hardware breakpoints, and supports 4-channel breakpoints at
most, and its lower 2 bits are valid. When configuring each channel breakpoint, you need to select the
corresponding channel through this register before configuration.
| Bit     | Name      | Access         |             | Description         |                | Reset Value  |
| ------- | --------- | -------------- | ----------- | ------------------- | -------------- | ------------ |
| [31:2]  | Reserved  | MRO  Reserved  |             |                     |                | 0            |
|         |           | The            | breakpoint  | channel  selection  | register       | is           |
|         |           | configured,    | that        | is,  after  the     | corresponding  |              |
[1:0]  TSELECT  MRW  channel is selected, the tdata1 and tdata2 registers  X
|     |     | can  | be  operated  | to  configure  | breakpoint  |     |
| --- | --- | ---- | ------------- | -------------- | ----------- | --- |
information.

Debug trigger data register 2(tdata2)
It is only valid for microprocessors that support hardware breakpoints, and is used to save the matching value
of the trigger.
| Bit     | Name    | Access                               |     | Description  |     | Reset Value  |
| ------- | ------- | ------------------------------------ | --- | ------------ | --- | ------------ |
| [31:0]  | TDATA2  | MRW  Used to save matching values.   |     |              |     | X            |

Trigger information register (tinfo)
| Bit  | Name  | Access  | Description  |     |     | Reset Value  |
| ---- | ----- | ------- | ------------ | --- | --- | ------------ |
[31:0]  tinfo  MRO  The trigger information register corresponds to  0x4
| V1.5  |     |     | 53  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | --- | --- | ------------------- |
the type value of the TDATA1 register, and the
type value is 2, and the second bit of tinfo is
set to 1.
Note: This register is only valid for V3F.

Debug control and status register (dcsr)
This register is used to control and record the running state of the debugging mode. Refer to Section 7.1 for
details.

Debug mode program pointer register (dpc)
This register is used to store the address of the next instruction to be executed after the microprocessor enters
the debugging mode, and its value is different according to the reasons for entering the debugging mode, and
the updating rules are also different. Refer to Section 7.1 for detailed description.

Debug trigger data register 1 (tdata1)
Valid  only  for  microprocessors  that  support  hardware  breakpoints,  microprocessors  that  support  only
instruction address and data address breakpoints, where bit TYPE in the tdata1 register has a fixed value of 2
and the other bits meet the definition of mcontrol in the debug standard.
|     | Bit  | Name  | Access  |     |     | Description  |     |     |     | Reset Value  |
| --- | ---- | ----- | ------- | --- | --- | ------------ | --- | --- | --- | ------------ |
[31:28]  TYPE  MRO  Breakpoint type definition, mcontrol type.  0x2
|     |     |     |     | 0:  Both  | machine  | mode  | and  debug  |     | mode  | can  |
| --- | --- | --- | --- | --------- | -------- | ----- | ----------- | --- | ----- | ---- |
modify the relevant register of the trigger;
|     | 27  | DMODE  | MRO  |            |       |       |              |      |           | 1   |
| --- | --- | ------ | ---- | ---------- | ----- | ----- | ------------ | ---- | --------- | --- |
|     |     |        |      | 1:  Debug  | mode  | only  | can  modify  | the  | relevant  |     |
register of the trigger.
|     |     |     |     | When  | MATCH=1,  | the  | maximum  | exponential  |     |     |
| --- | --- | --- | --- | ----- | --------- | ---- | -------- | ------------ | --- | --- |
[26:21]  MASKMAX  MRO  power  range  allowed  for  matching,  i.e.,  the  0x1F
maximum allowed matching range of 231 bytes.
|     | [20:13]  | Reserved  | MRO  | Reserved  |     |     |     |     |     | 0   |
| --- | -------- | --------- | ---- | --------- | --- | --- | --- | --- | --- | --- |
Processing mode when setting breakpoints:
|     |     |         |      | 0:  Enter  | breakpoint  |     | callback  | interrupt  | upon  |     |
| --- | --- | ------- | ---- | ---------- | ----------- | --- | --------- | ---------- | ----- | --- |
|     | 12  | ACTION  | MRW  |            |             |     |           |            |       | 0   |
triggering;
1: Enter debug mode upon triggering.
|     | [11:8]  | Reserved  | MRO  | Reserved  |     |     |     |     |     | 0   |
| --- | ------- | --------- | ---- | --------- | --- | --- | --- | --- | --- | --- |
Matching strategy configuration:
|     |     |     |     | 0:  Matches  |     | when  the  | trigger  | value  | equals  |     |
| --- | --- | --- | --- | ------------ | --- | ---------- | -------- | ------ | ------- | --- |
TDATA2;
7  MATCH  MRW  1: Matches when the trigger value matches the  0
high M bits of TDATA2, where M = 31 – N, and
N is the index of the first zero in TDATA2 (starting
from the least significant bit).
Trigger enable in M mode:
|     | 6   | M   | MRW  | 0: Disable trigger in M mode;  |     |     |     |     |     | 0   |
| --- | --- | --- | ---- | ------------------------------ | --- | --- | --- | --- | --- | --- |
1: Enable trigger in M mode.
|       | [5:4]  | Reserved  | MRO  | Reserved                   |     |     |     |     |     | 0   |
| ----- | ------ | --------- | ---- | -------------------------- | --- | --- | --- | --- | --- | --- |
|       | 3      | U         | MRW  | Trigger enable in U mode:  |     |     |     |     |     | 0   |
| V1.5  |        |           |      |                            | 54  |     |     |     |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
0: Disable trigger in U mode;
1: Enable trigger in U mode.
Instruction read address trigger enable:
| 2   | EXECUTE  | MRW  0: Disabled;  |     |     |     |     |     | 0   |
| --- | -------- | ------------------ | --- | --- | --- | --- | --- | --- |
1: Enabled.
Data write address trigger enable:
| 1   | STORE  | MRW  0: Disabled;  |     |     |     |     |     | 0   |
| --- | ------ | ------------------ | --- | --- | --- | --- | --- | --- |
1: Enabled.
Data read address trigger enable:
| 0   | LOAD  | MRW  0: Disabled;  |     |     |     |     |     | 0   |
| --- | ----- | ------------------ | --- | --- | --- | --- | --- | --- |
1: Enabled.

Debug mode staging register (dscratch0-1)
This group of registers is used for temporary storage of data in Debug mode.
| Bit     | Name      | Access                              |     | Description  |     |     | Reset Value  |     |
| ------- | --------- | ----------------------------------- | --- | ------------ | --- | --- | ------------ | --- |
| [31:0]  | dscratch  | DRW  Debug mode data staging value  |     |              |     |     |              | 0   |

Debug register 0 (dbgmcu_0)
| Bit  | Name  | Access  |     | Description  |     |     | Reset Value  |     |
| ---- | ----- | ------- | --- | ------------ | --- | --- | ------------ | --- |
The registration module stops working
when entering the debugging mode:
1. The registered peripheral stops working
[31:8]  dbg_mode_stop  MRW  when the core enters the debugging mode;  0
|     |     |     | 0:  The  | registered  | peripheral  | works  |     |     |
| --- | --- | --- | -------- | ----------- | ----------- | ------ | --- | --- |
normally when the core enters debugging
mode.
|     |     |     | The  registered  | module  | stops  | working  |     |     |
| --- | --- | --- | ---------------- | ------- | ------ | -------- | --- | --- |
when you enter the connected debugger:
|        |                |      | 1:  The  | registered  | peripheral  | stops  |     |     |
| ------ | -------------- | ---- | -------- | ----------- | ----------- | ------ | --- | --- |
| [7:0]  | dbg_link_stop  | MRW  |          |             |             |        |     | 0   |
working when you connect the debugger;
|     |     |     | 0:  The  | registered  | peripheral  | works  |     |     |
| --- | --- | --- | -------- | ----------- | ----------- | ------ | --- | --- |
normally when you connect the debugger.

Floating-point accumulation exception flag register (fflags)
| Bit     | Name      | Access  |           | Description  |     |     | Reset Value  |     |
| ------- | --------- | ------- | --------- | ------------ | --- | --- | ------------ | --- |
| [31:5]  | Reserved  | RO      | Reserved  |              |     |     |              | 0   |
| V1.5    |           |         | 55        |              |     |     |              |     |

| QingKeV3 Microprocessor Manual  |     |     |     |              |     |            |        |       | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | ------------ | --- | ---------- | ------ | ----- | ------------------- | --- |
|                                 |     |     |     | Accumulated  |     | exception  | flag,  | used  | to                  |     |
indicate exceptions that have occurred on
|     |     |     |     | floating  | point      | arithmetic     | instructions  |           |     |     |
| --- | --- | --- | --- | --------- | ---------- | -------------- | ------------- | --------- | --- | --- |
|     |     |     |     | since     | the  last  | reset  (reset  | or            | software  |     |     |
clear).
|     | [4:0]  | FFlags  | RW  | name  | bit  | description         |     |     |     | 0   |
| --- | ------ | ------- | --- | ----- | ---- | ------------------- | --- | --- | --- | --- |
|     |        |         |     | NV    | 4    | Illegal operation   |     |     |     |     |
|     |        |         |     | DZ    | 3    | Divide by zero      |     |     |     |     |
|     |        |         |     | OF    | 2    | Overflow up         |     |     |     |     |
|     |        |         |     | UF    | 1    | Overflow down       |     |     |     |     |
|     |        |         |     | NX    | 0    | Imprecise           |     |     |     |     |

Note: This register is valid only for V3F.

Floating-point rounding mode register (frm)
|     | Bit     | Name      | Access  |           |     | Description  |     |     | Reset Value  |     |
| --- | ------- | --------- | ------- | --------- | --- | ------------ | --- | --- | ------------ | --- |
|     | [31:3]  | Reserved  | RO      | Reserved  |     |              |     |     |              | 0   |
Floating-point operation rounding mode
register:
000: Round to the nearest value (round
to the even direction when it is in the
middle value);
001: Round to zero;
010: Round down;
011: Round up;
|     | [2:0]  | Frm  | RW  |       |           |          |          |        |     | 0   |
| --- | ------ | ---- | --- | ----- | --------- | -------- | -------- | ------ | --- | --- |
|     |        |      |     | 100:  | Rounding  | to  the  | nearest  | value  |     |     |
(rounding to the maximum magnitude
when it is in the middle value);
101, 110: Reserved;
|     |     |     |     | 111:      | Dynamic  | rounding:        | select  | the      |     |     |
| --- | --- | --- | --- | --------- | -------- | ---------------- | ------- | -------- | --- | --- |
|     |     |     |     | rounding  |          | mode  according  |         | to  the  |     |     |
configuration value of FRM bit in the
instruction.
Note: This register is valid only for V3F.

Floating-point control and status register (fcsr)
|     | Bit     | Name      | Access  |           |     | Description  |     |     | Reset Value  |     |
| --- | ------- | --------- | ------- | --------- | --- | ------------ | --- | --- | ------------ | --- |
|     | [31:8]  | Reserved  | RO      | Reserved  |     |              |     |     |              | 0   |
Floating-point operation rounding mode
|     | [7:5]  | Frm  | RW  |     |     |     |     |     |     | 0   |
| --- | ------ | ---- | --- | --- | --- | --- | --- | --- | --- | --- |
register.
|     | [4:0]  | FFlags  | RW  | Cumulative anomaly flag.  |     |     |     |     |     | 0   |
| --- | ------ | ------- | --- | ------------------------- | --- | --- | --- | --- | --- | --- |
Note: This register is valid only for V3F.

Fixed-point saturation status register (vxsat)
|       | Bit               | Name  | Access  |           |     | Description  |     |     |     | Reset Value  |
| ----- | ----------------- | ----- | ------- | --------- | --- | ------------ | --- | --- | --- | ------------ |
|       | [31:1]  Reserved  |       | URW     | Reserved  |     |              |     |     |     | 0            |
| V1.5  |                   |       |         | 56        |     |              |     |     |     |              |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | ------------------- |
Fixed-point saturation state register, which
is set to 1 when the result of fixed-point
| 0   | VXSAT  | URW  |     |     |     | 0   |
| --- | ------ | ---- | --- | --- | --- | --- |
operation must be output as a saturated
value.
Note: This register is valid only for V3V.

Fixed-point rounding register (vxrm)
| Bit     | Name      | Access  |              | Description  |           | Reset Value  |
| ------- | --------- | ------- | ------------ | ------------ | --------- | ------------ |
| [31:2]  | Reserved  | URW     | Reserved     |              |           | 0            |
|         |           |         | Fixed-point  | rounding     | register  | for          |
controlling rounding modes in fixed-point
operations:
00: Round toward nearest value (rounding
[1:0]  VXRM  URW  toward positive for intermediate values);  0
01: Round toward nearest value (rounding
toward even for intermediate values);
10: Round down;
11: Round toward odd.
Note: This register is valid only for V3V.

Vector control and status register (vcsr)
| Bit     | Name      | Access  |              | Description  |           | Reset Value  |
| ------- | --------- | ------- | ------------ | ------------ | --------- | ------------ |
| [31:3]  | Reserved  | URW     | Reserved     |              |           | 0            |
|         |           |         | Fixed-point  | rounding     | register  | for          |
controlling rounding modes in fixed-point
operations:
00: Round toward nearest value (rounding
[2:1]  VXRM  URW  toward positive for intermediate values);  0
01: Round toward nearest value (rounding
toward even for intermediate values);
10: Round down;
11: Round toward odd.
Fixed-point saturation state register, which
is set to 1 when the result of fixed-point
| 0   | VXSAT  | URW  |     |     |     | 0   |
| --- | ------ | ---- | --- | --- | --- | --- |
operation must be output as a saturated
value.
Note: This register is valid only for V3V.

Counter mask register (mcount_inhibit)
| Bit     | Name      | Access         |     | Description  |     | Reset Value  |
| ------- | --------- | -------------- | --- | ------------ | --- | ------------ |
| [31:3]  | Reserved  | MRO  Reserved  |     |              |     | 0            |
Instruction counter mask register:
1: Masks the instruction counter; MinStret and
| 2   | IR  | MRW  |     |     |     | 0   |
| --- | --- | ---- | --- | --- | --- | --- |
UnStret cease counting.
0: Does not mask the instruction counter; MinStret
| V1.5  |     |     | 57  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
and UnStret count normally.
| 1   | Reserved  | MRO  | Reserved  |     |     |     |     |     | 0   |
| --- | --------- | ---- | --------- | --- | --- | --- | --- | --- | --- |
Cycle counter disable register:
|     |     |      | 1:  Disables            |        | instruction  | counters;  mcycle  |              | and  |     |
| --- | --- | ---- | ----------------------- | ------ | ------------ | ------------------ | ------------ | ---- | --- |
| 0   | CY  | MRW  | ucycle cease counting;  |        |              |                    |              |      | 0   |
|     |     |      | 0:  Enables             | cycle  | counters;    | mcycle             | and  ucycle  |      |     |
count normally.
Note: This register is valid only for V3V.

Machine cycle counter register (mcycle)
| Bit  | Name  | Access  |     |     | Description  |     |     | Reset Value  |     |
| ---- | ----- | ------- | --- | --- | ------------ | --- | --- | ------------ | --- |
Machine cycle counter, used to count the clock
|     |     |     | cycles  | of  machine  | operation.  | Counting  | occurs  |     |     |
| --- | --- | --- | ------- | ------------ | ----------- | --------- | ------- | --- | --- |
[31:0]  MCYLE  MRW  when the CY bit in the mcount_inhibit register is  0
set to 0. Counting is disabled when the counter is
stopped in debug mode.
Note: This register is valid only for V3V.

Machine instruction counter register (minstret)
| Bit  | Name  | Access  |              |                   | Description  |                      |        | Reset Value  |     |
| ---- | ----- | ------- | ------------ | ----------------- | ------------ | -------------------- | ------ | ------------ | --- |
|      |       |         | Instruction  | count             | register,    | used  to             | count  | the          |     |
|      |       |         | number       | of  instructions  |              | correctly  executed  |        | and          |     |
completed by the machine. Counting occurs when
| [31:0]  | MINSTRET  | MRW  |     |     |     |     |     |     | 0   |
| ------- | --------- | ---- | --- | --- | --- | --- | --- | --- | --- |
the IR bit in the mcount_inhibit register is set to 0.
Counting is disabled when the counter is stopped
in debug mode.
Note: This register is valid only for V3V.

User mode cycle counter register (ucycle)
| Bit  | Name  | Access  |     |     | Description  |     |     | Reset Value  |     |
| ---- | ----- | ------- | --- | --- | ------------ | --- | --- | ------------ | --- |
User Mode Cycle Counter, used to count the clock
|     |     |     | cycles  | of  machine  |     | operation.  Counting  | occurs  |     |     |
| --- | --- | --- | ------- | ------------ | --- | --------------------- | ------- | --- | --- |
[31:0]  UCYLE  RO  when the CY bit in the mcount_inhibit register is  0
set to 0. Counting is disabled when the counter is
stopped in debug mode.
Note: 1. User Mode Instruction Count Register, functions identically to the minstret register, used to read the
number of instructions correctly executed by the machine in user mode.
2. This register is only valid for V3V.

User mode instruction counter register (uinstret)
| Bit     | Name      | Access  |              |                   | Description  |                      |        | Reset Value  |     |
| ------- | --------- | ------- | ------------ | ----------------- | ------------ | -------------------- | ------ | ------------ | --- |
|         |           |         | Instruction  | count             | register,    | used  to             | count  | the          |     |
|         |           |         | number       | of  instructions  |              | correctly  executed  |        | and          |     |
| [31:0]  | uinstret  | RO      |              |                   |              |                      |        |              | 0   |
completed by the machine. Counting occurs when
the IR bit in the MCOUNT_INHIBIT register is
| V1.5  |     |     |     | 58  |     |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | ------------------- | --- |
set to 0. Counting is disabled when the counter is
stopped in debug mode.
Note: 1. User Mode Instruction Counter, with functionality identical to the minstret register, is used to read
the number of instructions correctly executed by the machine in user mode.
2. This register is only valid for V3V.

Element body length register (vl)
| Bit               | Name  | Access  |           | Description  |     | Reset Value  |     |
| ----------------- | ----- | ------- | --------- | ------------ | --- | ------------ | --- |
| [31:7]  Reserved  |       | URW     | Reserved  |              |     |              | 0   |
Vector Length Register, used to store the
|     |     |     | element  | length  employed  | in  operations.  |     |     |
| --- | --- | --- | -------- | ----------------- | ---------------- | --- | --- |
The register value can only be modified by
| [6:0]  VL  |     | URO  |     |     |     |     | 0   |
| ---------- | --- | ---- | --- | --- | --- | --- | --- |
the vset instruction. Assignment rules are
|     |     |     | detailed  | in  the  RISC-V  | “V”  | vector  |     |
| --- | --- | --- | --------- | ---------------- | ---- | ------- | --- |
Extension 1.0 manual.
Note: This register is valid only for V3V.

Vector data type register (vtype)
| Bit               | Name  | Access  |                | Description       |           | Reset Value  |     |
| ----------------- | ----- | ------- | -------------- | ----------------- | --------- | ------------ | --- |
| [31:6]  Reserved  |       | URW     | Reserved       |                   |           |              | 0   |
|                   |       |         | Vector  width  | register,  which  | is  used  | to           |     |
select the widths of vectors and elements.
|     |     |     | Only  the  | VSET  instruction  | can  | be  |     |
| --- | --- | --- | ---------- | ------------------ | ---- | --- | --- |
configured:
000: 8bit;
001: 16bit;
| [5:3]  VSEW  |     | URO  |     |     |     |     | 0   |
| ------------ | --- | ---- | --- | --- | --- | --- | --- |
010: 32bit;
011: 64bit;
1xx: Reserved.
Note: configuring eew to 64bit requires
|     |     |     | enabling  | the  relevant  | configuration  | of  |     |
| --- | --- | --- | --------- | -------------- | -------------- | --- | --- |
vcontrol.
Vector Set Register, used to select grouping
|     |     |     | information   | for  vector  | operations,  |       |     |
| --- | --- | --- | ------------- | ------------ | ------------ | ----- | --- |
|     |     |     | configurable  | only  via    | the          | VSET  |     |
instruction:
000: 1;
001: 2;
| [2:0]  VLMUL  |     | URO  |     |     |     |     | 0   |
| ------------- | --- | ---- | --- | --- | --- | --- | --- |
010: 4;
011: 8;
100: Reserved;
101: 1/8;
110: 1/4;
111: 1/2.
| V1.5  |     |     | 59  |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | --- | --- | ------------------- |
Note: This register is valid only for V3V.

Vector register byte length register (vlenb)
| Bit  | Name  | Access  |     | Description  |     | Reset Value  |
| ---- | ----- | ------- | --- | ------------ | --- | ------------ |
Vector register group byte length register,
this kernel vector register has a length of
| [31:0]  VLENB  |     | URO  |     |     |     | 0x8  |
| -------------- | --- | ---- | --- | --- | --- | ---- |
64bit and a byte length of 8, with a fixed
value.
Note: This register is valid only for V3V.

Custom vector control register (vcontrol)
| Bit  | Name  | Access  |     | Description  |     | Reset Value  |
| ---- | ----- | ------- | --- | ------------ | --- | ------------ |
Vector instruction exception flag, which is
set when the vector instruction generates an
31  VEC_ILL  URW1C  exception.  When  NMI_EN  register  is  0
enabled, the vector exception will enter
NMI exception.
| [30:24]  Reserved  |     | URO  | Reserved  |     |     | 0   |
| ------------------ | --- | ---- | --------- | --- | --- | --- |
Interrupt nesting depth:
00: No interrupt;
[23:22]  NEST_INT_PTR  URO  01: Level 1 interrupt nesting;  0
10: Level 2 interrupt nesting;
11: Level 3 interrupt nesting.
Vector pipeline interrupt nesting depth:
00: No interrupt;
01: Level 1 interrupt nesting;
10: Level 2 interrupt nesting;
[21:20]  VEC_INT_PTR  URW  11: Level 3 interrupt nesting.  0
When vector instructions are not used in
|     |     |     | the  interrupt  | service  routine,  | the  vector  |     |
| --- | --- | --- | --------------- | ------------------ | ------------ | --- |
pipeline interrupt nesting depth retains its
previous value.
Vector register status register, comprising 4
bits corresponding to a maximum vector
register status depth of four levels. A value
of 1 indicates that the vector register status
| [19:16]  VEC_REG_STATE  |     | URW  |     |     |     | 0   |
| ----------------------- | --- | ---- | --- | --- | --- | --- |
at the corresponding interrupt depth is in a
dirty state, meaning a vector instruction has
been executed, resulting in a change to the
vector register value.
| [15:10]  Reserved  |     | URO  | Reserved  |     |     | 0   |
| ------------------ | --- | ---- | --------- | --- | --- | --- |
Vector exception information save enabled.
|                  |     |      | When  set,  | the  VCAUSE          | and  VTVAL  |     |
| ---------------- | --- | ---- | ----------- | -------------------- | ----------- | --- |
| 9  VEC_ILL_INFO  |     | URW  |             |                      |             | 1   |
|                  |     |      | registers   | will  store  vector  | exception   |     |
information.
| V1.5  |     |     | 60  |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |      |                            |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | ---- | -------------------------- | --- | --- | --- | ------------------- |
| 8  Reserved                     |     | URO  | Reserved, cannot write 0.  |     |     |     | 1                   |
| [7:4]  Reserved                 |     | URO  | Reserved.                  |     |     |     | 0                   |
Force write sequence enabled, forces write
3  LSU_WAX_ON  URW  operations  to  execute  in  order  of  1
instruction.
Force read sequence enabled, forces read
2  LSU_RAW_ON  URW  operations  to  be  executed  in  order  of  1
instruction.
Vector register hardware stack protection
ABI configuration registers:
|     |     |     | 1:  Call-variant  | ABI,  | hardware  | stack  |     |
| --- | --- | --- | ----------------- | ----- | --------- | ------ | --- |
protection for registers V0 and V8-V23;
| 1  VEC_HW_PP_ABI  |     | URW  |               |             |           |        | 0   |
| ----------------- | --- | ---- | ------------- | ----------- | --------- | ------ | --- |
|                   |     |      | 0:  Standard  | ABI  call,  | hardware  | stack  |     |
protection for registers V0-V31.
Note: For ABI calls, refer to the RISC-V
ABIS SPECIFICATION V1.1.
Vector register hardware stack protection
enable:
| 0  VEC_HW_PP_EN  |     | URW  |     |     |     |     | 0   |
| ---------------- | --- | ---- | --- | --- | --- | --- | --- |
1: Enable vector stack protection;
0: Disable vector stack protection.
Note: This register is valid only for V3V.

Custom vector register stack base address register (vppaddr)
| Bit  | Name  | Access  |     | Description  |     |     | Reset Value  |
| ---- | ----- | ------- | --- | ------------ | --- | --- | ------------ |
Vector register stack protection memory
|     |     |     | address  register  | used  | to  configure  | the  |     |
| --- | --- | --- | ------------------ | ----- | -------------- | ---- | --- |
[31:0]  VPPADDR  URW  vector  register  stack  protection  address.  0x20000000
The lower 10 bits are fixed at 0. Memory is
1KB-aligned.
Note: This register is valid only for V3V.

Custom vector exception source register (vcause)
| Bit  | Name  | Access  |     | Description  |     |     | Reset Value  |
| ---- | ----- | ------- | --- | ------------ | --- | --- | ------------ |
Vector exception cause register
|                 |     |      | When  VEC_ILL_INFO  |              | is  enabled,  | this    |     |
| --------------- | --- | ---- | ------------------- | ------------ | ------------- | ------- | --- |
|                 |     |      | register            | stores  the  | cause  of     | vector  |     |
| [31:0]  VCAUSE  |     | URO  | exceptions.         |              |               |         | 0   |
Note: Debug mode causes loss of exception
cause information and cannot be used in
debug mode.
Note: This register is valid only for V3V.

custom vector exception trap information register (vtval)
| Bit  | Name  | Access  |     | Description  |     |     | Reset Value  |
| ---- | ----- | ------- | --- | ------------ | --- | --- | ------------ |
[31:0]  VTVAL  URO  Vector  instruction  self-trap  information  0
| V1.5  |     |     | 61  |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- |

QingKeV3 Microprocessor Manual https://wch-ic.com
register
When VEC_ILL_INFO is enabled, this
register stores the PC value at the time of
the vector exception.
Note: Debug mode causes loss of exception
cause information and cannot be used in
debug mode.
Note: This register is valid only for V3V.
8.3 User-defined CSR Register
User mode global interrupt enable register (gintenr)
This register is used to control the enable and mask of global interrupt. The enable and mask of global interrupt
in machine mode can be controlled by the MIE and MPIE bits in mstatus, but this register cannot be operated
in user mode. While the global interrupt enable register gintenr is the mapping of MIE and MPIE in mstatus.
In user mode, gintenr can be used to set and clear MIE and MPIE, as described in Section 3.2 for details.
Note: Global interrupts do not include unmasked interrupts NMI and exceptions.
Interrupt system control register (intsyscr)
This register is mainly used to configure interrupt nesting depth, hardware stack pressing and other related
functions, as described in Section 3.2 for details.
Microprocessor configuration register (corecfgr)
This register is used to control whether the NMI interrupt is allowed after the interrupt overflows, and whether
the interrupt request is cleared when the fence instruction is executed. Please refer to Section 3.2 for the
specific definition.
Interrupt nested control register (inestcr)
This register is used to indicate the interrupt nesting state and whether it overflows or not, and to control the
maximum nesting level. Please refer to Section 3.2 for the specific definition.
V1.5 62

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |
| ------------------------------- | --- | --- | --- | ------------------- |
Chapter 9 V3 Series Core Model and Subversion Description
V3B, V3C and V3F versions are mainly used in V3 processors, of which V3C version is the most used.
|     | V3A  | V3B  | V3C  | V3F  |
| --- | ---- | ---- | ---- | ---- |
|     | √    | ×    | ×    | √    |
AMO
MRS  (Shift  multiplication
|     | ×   | √   | ×   | ×   |
| --- | --- | --- | --- | --- |
custom instruction)
| Interrupt priority register width  | 4   | 2   | 3   | 3   |
| ---------------------------------- | --- | --- | --- | --- |
| PMP                                | √   | ×   | √   | √   |
|                                    | ×   | ×   | √   | √   |
LSU_TRIGGER

MIMPID is a csr register for recording sub-version information:
mimpid[7:0]=1; CH32M030, CH585 only.
mimpid[7:0]=2; Support software reset (i.e., Interrupt Module FPIC_CFGR register)
             Software interrupt (Add STK_CTLR[31] as software interrupt flag bit, same function as STK_
SR[31]);
             Support interrupt nested enable and mask;
             Support preemption priority bit width configuration;
mimpid[7:0]=3; new custom delay instruction;
             Support zicond instruction.

|       |     |     |     |     |
| ----- | --- | --- | --- | --- |
| V1.5  |     | 63  |     |     |

| QingKeV3 Microprocessor Manual  |     |     |     |     |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | --- | --- | --- | --- | ------------------- | --- |
Chapter 10 Appendix 1 Delay Instructions
Custom delay instructions are precise delay commands that incur a single instruction overhead. They can delay
specified matching instructions with clock-cycle precision, eliminating uncertainties caused by instruction
jumps during timer operations or delays. These instructions automatically match instruction codes, configure
delay cycles, and apply clock division factors, offering flexible usage. They provide significant advantages in
applications requiring precise control, process management, and low-overhead delays.

Delay instruction coding format
|     | Bit  |     | Name  |     |     | Description  |     |     |     |
| --- | ---- | --- | ----- | --- | --- | ------------ | --- | --- | --- |
Delay immediate number, depending on configuration,
[31:20]  imm  can indicate the number of clock cycles delayed or
divided.
|     | [19:15]  | rs1    |     |     | Match rs1 register code.  |     |     |     |     |
| --- | -------- | ------ | --- | --- | ------------------------- | --- | --- | --- | --- |
|     | [14:12]  | func3  |     |     | 001b。                     |     |     |     |     |
Matching instruction types; instructions matching types
cannot execute while delay persists
000: Reserved;
001: Matches load instructions;
010: Matches store instructions;
|     | [11：9]  | match  |     |     | 011: Matches load/store instructions;  |     |     |     |     |
| --- | ------- | ------ | --- | --- | -------------------------------------- | --- | --- | --- | --- |
100: Matches delay instructions;
101: Reserved;
110: Matches all instructions (pipeline stalls);
|     |     |     |     |     | 111:  Matches  | instructions  | with  specified  | func3  | and  |
| --- | --- | --- | --- | --- | -------------- | ------------- | ---------------- | ------ | ---- |
opcode (matching values configured by CSR register).
Count using the main frequency clock or the divided
frequency clock
8  div  1: Count using the divided frequency cycle (division
factor configured by the CSR register);
0: Count using the main frequency cycle.
Number of delay options:
|     | 7   | sel  |     |     | 1: rs1+rs2;  |     |     |     |     |
| --- | --- | ---- | --- | --- | ------------ | --- | --- | --- | --- |
0: rs1 + imm.
|     | [6:0]  | func7  |     |     | 0001011b.  |     |     |     |     |
| --- | ------ | ------ | --- | --- | ---------- | --- | --- | --- | --- |
Note: Read/write instructions using the SP register as the base address are not subject to matching load/store
instructions (match=001/010/011).

Delay instruction control register (U_NONS_DLY_0)
CSR address: 0x8C0
|     | Bit  | Name  |     | Access  |     | Description  |     | Reset Value  |     |
| --- | ---- | ----- | --- | ------- | --- | ------------ | --- | ------------ | --- |
Matching instruction parameter mask when
| [31:24]  | dly_mask  |     |     | URW  |     |     |     |     | x   |
| -------- | --------- | --- | --- | ---- | --- | --- | --- | --- | --- |
dly instruction match=111.
The {func, opcode} matched when the dly
| [23:16]  | dly_data  |     |     | URW  |     |     |     |     | x   |
| -------- | --------- | --- | --- | ---- | --- | --- | --- | --- | --- |
instruction has match=111.
| V1.5  |     |     |     |     | 64  |     |     |     |     |
| ----- | --- | --- | --- | --- | --- | --- | --- | --- | --- |

| QingKeV3 Microprocessor Manual  |     |     |     | https://wch-ic.com  |     |
| ------------------------------- | --- | --- | --- | ------------------- | --- |
15  CPU_dly_busy_sta  URO  The flag that that core is in a delay state.  0
Timeout information, there is no matching
| 14  dly_tout  | RW1Z  |     |     |     | 0   |
| ------------- | ----- | --- | --- | --- | --- |
instruction request within the delay range.
Delay interrupt request. If timeout occurs
13  dly_ie  URW  after enabling, a software interrupt request  0
will be generated.
1: Both machine mode and user mode are
permitted to match.
| 12  dly_priv  | URW  |     |     |     | 0   |
| ------------- | ---- | --- | --- | --- | --- |
0: Delay operations executed in user mode
do not match instructions in machine mode.
| [11:10]  Reserved  | URO  Reserved  |           |            |           | 0   |
| ------------------ | -------------- | --------- | ---------- | --------- | --- |
|                    | Delay          | counting  | frequency  | division  |     |
coefficient configuration, if configured as
| [9:0]  dly_freq  | URW  |     |     |     | x   |
| ---------------- | ---- | --- | --- | --- | --- |
the main frequency clock frequency, can
realize the us unit delay.

| V1.5  |     | 65  |     |     |     |
| ----- | --- | --- | --- | --- | --- |