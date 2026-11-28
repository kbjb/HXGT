
========================================================================

** ELF Header Information

    File Name: C:\Users\zhangqingqing\Documents\ChatGPT\SXGJC 2\tmp\cg40_work\delay_before_O3.o

    Machine class: ELFCLASS32 (32-bit)
    Data encoding: ELFDATA2LSB (Little endian)
    Header version: EV_CURRENT (Current version)
    Operating System ABI: none
    ABI Version: 0
    File Type: ET_REL (Relocatable object) (1)
    Machine: EM_ARM (ARM)

    Entry offset (in SHF_ENTRYSECT section): 0x00000000
    Flags: None (0x05000000)

    ARM ELF revision: 5 (ABI version 2)

    Built with
    Component: ARM Compiler 5.06 update 7 (build 960) Tool: armcc [4d365d]

    Header size: 52 bytes (0x34)
    Program header entry size: 0 bytes (0x0)
    Section header entry size: 40 bytes (0x28)

    Program header entries: 0
    Section header entries: 11

    Program header offset: 0 (0x00000000)
    Section header offset: 2576 (0x00000a10)

    Section header string table index: 8

========================================================================

** Section #1 '.text' (SHT_PROGBITS) [SHF_ALLOC + SHF_EXECINSTR]
    Size   : 294 bytes (alignment 4)
    Address: 0x00000000

    $t
    .text
    LM9507_DELAY
        0x00000000:    1e40        @.      SUBS     r0,r0,#1
        0x00000002:    d2fd        ..      BCS      LM9507_DELAY ; 0x0
        0x00000004:    4770        pG      BX       lr
    LM9507_INIT
        0x00000006:    b510        ..      PUSH     {r4,lr}
        0x00000008:    4604        .F      MOV      r4,r0
        0x0000000a:    6800        .h      LDR      r0,[r0,#0]
        0x0000000c:    68c1        .h      LDR      r1,[r0,#0xc]
        0x0000000e:    4788        .G      BLX      r1
        0x00000010:    6860        `h      LDR      r0,[r4,#4]
        0x00000012:    68c1        .h      LDR      r1,[r0,#0xc]
        0x00000014:    4788        .G      BLX      r1
        0x00000016:    68a0        .h      LDR      r0,[r4,#8]
        0x00000018:    68c1        .h      LDR      r1,[r0,#0xc]
        0x0000001a:    4788        .G      BLX      r1
        0x0000001c:    68e0        .h      LDR      r0,[r4,#0xc]
        0x0000001e:    68c1        .h      LDR      r1,[r0,#0xc]
        0x00000020:    e8bd4010    ...@    POP      {r4,lr}
        0x00000024:    4708        .G      BX       r1
    LM9507_WRITE
        0x00000026:    e92d41f0    -..A    PUSH     {r4-r8,lr}
        0x0000002a:    240f        .$      MOVS     r4,#0xf
        0x0000002c:    460f        .F      MOV      r7,r1
        0x0000002e:    4605        .F      MOV      r5,r0
        0x00000030:    f05f0601    _...    MOVS.W   r6,#1
        0x00000034:    fa06f004    ....    LSL      r0,r6,r4
        0x00000038:    4238        8B      TST      r0,r7
        0x0000003a:    d001        ..      BEQ      {pc}+0x6 ; 0x40
        0x0000003c:    2101        .!      MOVS     r1,#1
        0x0000003e:    e000        ..      B        {pc}+0x4 ; 0x42
        0x00000040:    2100        .!      MOVS     r1,#0
        0x00000042:    68a8        .h      LDR      r0,[r5,#8]
        0x00000044:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000046:    4790        .G      BLX      r2
        0x00000048:    6828        (h      LDR      r0,[r5,#0]
        0x0000004a:    2101        .!      MOVS     r1,#1
        0x0000004c:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x0000004e:    4790        .G      BLX      r2
        0x00000050:    2008        .       MOVS     r0,#8
        0x00000052:    1e40        @.      SUBS     r0,r0,#1
        0x00000054:    d2fd        ..      BCS      {pc}-0x2 ; 0x52
        0x00000056:    6828        (h      LDR      r0,[r5,#0]
        0x00000058:    2100        .!      MOVS     r1,#0
        0x0000005a:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x0000005c:    4790        .G      BLX      r2
        0x0000005e:    2008        .       MOVS     r0,#8
        0x00000060:    1e40        @.      SUBS     r0,r0,#1
        0x00000062:    d2fd        ..      BCS      {pc}-0x2 ; 0x60
        0x00000064:    1e64        d.      SUBS     r4,r4,#1
        0x00000066:    b224        $.      SXTH     r4,r4
        0x00000068:    2c00        .,      CMP      r4,#0
        0x0000006a:    dae3        ..      BGE      {pc}-0x36 ; 0x34
        0x0000006c:    e8bd81f0    ....    POP      {r4-r8,pc}
    LM9507_READ
        0x00000070:    e92d41f0    -..A    PUSH     {r4-r8,lr}
        0x00000074:    2400        .$      MOVS     r4,#0
        0x00000076:    4606        .F      MOV      r6,r0
        0x00000078:    4625        %F      MOV      r5,r4
        0x0000007a:    f64f77ff    O..w    MOV      r7,#0xffff
        0x0000007e:    6830        0h      LDR      r0,[r6,#0]
        0x00000080:    2101        .!      MOVS     r1,#1
        0x00000082:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000084:    4790        .G      BLX      r2
        0x00000086:    68f0        .h      LDR      r0,[r6,#0xc]
        0x00000088:    ea070444    ..D.    AND      r4,r7,r4,LSL #1
        0x0000008c:    6a01        .j      LDR      r1,[r0,#0x20]
        0x0000008e:    4788        .G      BLX      r1
        0x00000090:    2801        .(      CMP      r0,#1
        0x00000092:    d101        ..      BNE      {pc}+0x6 ; 0x98
        0x00000094:    1c64        d.      ADDS     r4,r4,#1
        0x00000096:    b2a4        ..      UXTH     r4,r4
        0x00000098:    6830        0h      LDR      r0,[r6,#0]
        0x0000009a:    2100        .!      MOVS     r1,#0
        0x0000009c:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x0000009e:    4790        .G      BLX      r2
        0x000000a0:    2008        .       MOVS     r0,#8
        0x000000a2:    1e40        @.      SUBS     r0,r0,#1
        0x000000a4:    d2fd        ..      BCS      {pc}-0x2 ; 0xa2
        0x000000a6:    1c6d        m.      ADDS     r5,r5,#1
        0x000000a8:    b2ed        ..      UXTB     r5,r5
        0x000000aa:    2d10        .-      CMP      r5,#0x10
        0x000000ac:    d3e7        ..      BCC      {pc}-0x2e ; 0x7e
        0x000000ae:    4620         F      MOV      r0,r4
        0x000000b0:    e7dc        ..      B        {pc}-0x44 ; 0x6c
    LM95071_READTEMP
        0x000000b2:    b570        p.      PUSH     {r4-r6,lr}
        0x000000b4:    4604        .F      MOV      r4,r0
        0x000000b6:    6840        @h      LDR      r0,[r0,#4]
        0x000000b8:    2100        .!      MOVS     r1,#0
        0x000000ba:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000bc:    4790        .G      BLX      r2
        0x000000be:    69a1        .i      LDR      r1,[r4,#0x18]
        0x000000c0:    4620         F      MOV      r0,r4
        0x000000c2:    4788        .G      BLX      r1
        0x000000c4:    4605        .F      MOV      r5,r0
        0x000000c6:    6860        `h      LDR      r0,[r4,#4]
        0x000000c8:    2101        .!      MOVS     r1,#1
        0x000000ca:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000cc:    4790        .G      BLX      r2
        0x000000ce:    08a8        ..      LSRS     r0,r5,#2
        0x000000d0:    0080        ..      LSLS     r0,r0,#2
        0x000000d2:    b200        ..      SXTH     r0,r0
        0x000000d4:    1080        ..      ASRS     r0,r0,#2
        0x000000d6:    f7fffffe    ....    BL       __aeabi_i2f
        0x000000da:    e8bd4070    ..p@    POP      {r4-r6,lr}
        0x000000de:    f06f0104    o...    MVN      r1,#4
        0x000000e2:    f7ffbffe    ....    B.W      __ARM_scalbnf
    LM95071_SHOUTDOWN
        0x000000e6:    b510        ..      PUSH     {r4,lr}
        0x000000e8:    4604        .F      MOV      r4,r0
        0x000000ea:    6840        @h      LDR      r0,[r0,#4]
        0x000000ec:    2100        .!      MOVS     r1,#0
        0x000000ee:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000f0:    4790        .G      BLX      r2
        0x000000f2:    6962        bi      LDR      r2,[r4,#0x14]
        0x000000f4:    21ff        .!      MOVS     r1,#0xff
        0x000000f6:    4620         F      MOV      r0,r4
        0x000000f8:    4790        .G      BLX      r2
        0x000000fa:    6860        `h      LDR      r0,[r4,#4]
        0x000000fc:    2101        .!      MOVS     r1,#1
        0x000000fe:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000100:    e8bd4010    ...@    POP      {r4,lr}
        0x00000104:    4710        .G      BX       r2
    LM9507_STR_INIT
        0x00000106:    f2af1101    ....    ADR      r1,{pc}-0xff ; 0x7
        0x0000010a:    6101        .a      STR      r1,[r0,#0x10]
        0x0000010c:    f2af01e9    ....    ADR      r1,{pc}-0xe5 ; 0x27
        0x00000110:    6141        Aa      STR      r1,[r0,#0x14]
        0x00000112:    f2af01a3    ....    ADR      r1,{pc}-0xa1 ; 0x71
        0x00000116:    6181        .a      STR      r1,[r0,#0x18]
        0x00000118:    f2af0169    ..i.    ADR      r1,{pc}-0x65 ; 0xb3
        0x0000011c:    61c1        .a      STR      r1,[r0,#0x1c]
        0x0000011e:    f2af0139    ..9.    ADR      r1,{pc}-0x37 ; 0xe7
        0x00000122:    6201        .b      STR      r1,[r0,#0x20]
        0x00000124:    e7fe        ..      B        LM9507_INIT ; 0x6 Section #1

** Section #6 '.rel.text' (SHT_REL)
    Size   : 24 bytes (alignment 4)
    Symbol table #5 '.symtab'
    3 relocations applied to section #1 '.text'


** Section #2 '.arm_vfe_header' (SHT_PROGBITS)
    Size   : 4 bytes (alignment 4)


** Section #3 '.comment' (SHT_PROGBITS)
    Size   : 968 bytes


** Section #4 '.debug_frame' (SHT_PROGBITS)
    Size   : 292 bytes


** Section #7 '.rel.debug_frame' (SHT_REL)
    Size   : 112 bytes (alignment 4)
    Symbol table #5 '.symtab'
    14 relocations applied to section #4 '.debug_frame'


** Section #5 '.symtab' (SHT_SYMTAB)
    Size   : 256 bytes (alignment 4)
    String table #9 '.strtab'
    Last local symbol no. 5


** Section #8 '.shstrtab' (SHT_STRTAB)
    Size   : 95 bytes


** Section #9 '.strtab' (SHT_STRTAB)
    Size   : 405 bytes


** Section #10 '.ARM.attributes' (SHT_ARM_ATTRIBUTES)
    Size   : 72 bytes


