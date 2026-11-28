
========================================================================

** ELF Header Information

    File Name: C:\Users\zhangqingqing\Documents\ChatGPT\SXGJC 2\tmp\cg40_work\delay_after_O2.o

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
    Section header offset: 2660 (0x00000a64)

    Section header string table index: 8

========================================================================

** Section #1 '.text' (SHT_PROGBITS) [SHF_ALLOC + SHF_EXECINSTR]
    Size   : 302 bytes (alignment 4)
    Address: 0x00000000

    $t
    .text
    LM9507_DELAY
        0x00000000:    b508        ..      PUSH     {r3,lr}
        0x00000002:    2800        .(      CMP      r0,#0
        0x00000004:    9000        ..      STR      r0,[sp,#0]
        0x00000006:    d002        ..      BEQ      {pc}+0x8 ; 0xe
        0x00000008:    bf00        ..      NOP      
        0x0000000a:    1e40        @.      SUBS     r0,r0,#1
        0x0000000c:    e7f9        ..      B        {pc}-0xa ; 0x2
        0x0000000e:    bd08        ..      POP      {r3,pc}
    LM9507_INIT
        0x00000010:    b510        ..      PUSH     {r4,lr}
        0x00000012:    4604        .F      MOV      r4,r0
        0x00000014:    6800        .h      LDR      r0,[r0,#0]
        0x00000016:    68c1        .h      LDR      r1,[r0,#0xc]
        0x00000018:    4788        .G      BLX      r1
        0x0000001a:    6860        `h      LDR      r0,[r4,#4]
        0x0000001c:    68c1        .h      LDR      r1,[r0,#0xc]
        0x0000001e:    4788        .G      BLX      r1
        0x00000020:    68a0        .h      LDR      r0,[r4,#8]
        0x00000022:    68c1        .h      LDR      r1,[r0,#0xc]
        0x00000024:    4788        .G      BLX      r1
        0x00000026:    68e0        .h      LDR      r0,[r4,#0xc]
        0x00000028:    68c1        .h      LDR      r1,[r0,#0xc]
        0x0000002a:    e8bd4010    ...@    POP      {r4,lr}
        0x0000002e:    4708        .G      BX       r1
    LM9507_WRITE
        0x00000030:    e92d41f0    -..A    PUSH     {r4-r8,lr}
        0x00000034:    240f        .$      MOVS     r4,#0xf
        0x00000036:    460e        .F      MOV      r6,r1
        0x00000038:    4605        .F      MOV      r5,r0
        0x0000003a:    2701        .'      MOVS     r7,#1
        0x0000003c:    fa07f004    ....    LSL      r0,r7,r4
        0x00000040:    4230        0B      TST      r0,r6
        0x00000042:    d001        ..      BEQ      {pc}+0x6 ; 0x48
        0x00000044:    2101        .!      MOVS     r1,#1
        0x00000046:    e000        ..      B        {pc}+0x4 ; 0x4a
        0x00000048:    2100        .!      MOVS     r1,#0
        0x0000004a:    68a8        .h      LDR      r0,[r5,#8]
        0x0000004c:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x0000004e:    4790        .G      BLX      r2
        0x00000050:    6828        (h      LDR      r0,[r5,#0]
        0x00000052:    2101        .!      MOVS     r1,#1
        0x00000054:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000056:    4790        .G      BLX      r2
        0x00000058:    200a        .       MOVS     r0,#0xa
        0x0000005a:    f7fffffe    ....    BL       LM9507_DELAY ; 0x0 Section #1
        0x0000005e:    6828        (h      LDR      r0,[r5,#0]
        0x00000060:    2100        .!      MOVS     r1,#0
        0x00000062:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000064:    4790        .G      BLX      r2
        0x00000066:    200a        .       MOVS     r0,#0xa
        0x00000068:    f7fffffe    ....    BL       LM9507_DELAY ; 0x0 Section #1
        0x0000006c:    1e64        d.      SUBS     r4,r4,#1
        0x0000006e:    b224        $.      SXTH     r4,r4
        0x00000070:    2c00        .,      CMP      r4,#0
        0x00000072:    dae3        ..      BGE      {pc}-0x36 ; 0x3c
        0x00000074:    e8bd81f0    ....    POP      {r4-r8,pc}
    LM9507_READ
        0x00000078:    e92d41f0    -..A    PUSH     {r4-r8,lr}
        0x0000007c:    2400        .$      MOVS     r4,#0
        0x0000007e:    4606        .F      MOV      r6,r0
        0x00000080:    4625        %F      MOV      r5,r4
        0x00000082:    f64f77ff    O..w    MOV      r7,#0xffff
        0x00000086:    6830        0h      LDR      r0,[r6,#0]
        0x00000088:    2101        .!      MOVS     r1,#1
        0x0000008a:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x0000008c:    4790        .G      BLX      r2
        0x0000008e:    68f0        .h      LDR      r0,[r6,#0xc]
        0x00000090:    ea070444    ..D.    AND      r4,r7,r4,LSL #1
        0x00000094:    6a01        .j      LDR      r1,[r0,#0x20]
        0x00000096:    4788        .G      BLX      r1
        0x00000098:    2801        .(      CMP      r0,#1
        0x0000009a:    d101        ..      BNE      {pc}+0x6 ; 0xa0
        0x0000009c:    1c64        d.      ADDS     r4,r4,#1
        0x0000009e:    b2a4        ..      UXTH     r4,r4
        0x000000a0:    6830        0h      LDR      r0,[r6,#0]
        0x000000a2:    2100        .!      MOVS     r1,#0
        0x000000a4:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000a6:    4790        .G      BLX      r2
        0x000000a8:    200a        .       MOVS     r0,#0xa
        0x000000aa:    f7fffffe    ....    BL       LM9507_DELAY ; 0x0 Section #1
        0x000000ae:    1c6d        m.      ADDS     r5,r5,#1
        0x000000b0:    b2ed        ..      UXTB     r5,r5
        0x000000b2:    2d10        .-      CMP      r5,#0x10
        0x000000b4:    d3e7        ..      BCC      {pc}-0x2e ; 0x86
        0x000000b6:    4620         F      MOV      r0,r4
        0x000000b8:    e7dc        ..      B        {pc}-0x44 ; 0x74
    LM95071_READTEMP
        0x000000ba:    b570        p.      PUSH     {r4-r6,lr}
        0x000000bc:    4604        .F      MOV      r4,r0
        0x000000be:    6840        @h      LDR      r0,[r0,#4]
        0x000000c0:    2100        .!      MOVS     r1,#0
        0x000000c2:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000c4:    4790        .G      BLX      r2
        0x000000c6:    69a1        .i      LDR      r1,[r4,#0x18]
        0x000000c8:    4620         F      MOV      r0,r4
        0x000000ca:    4788        .G      BLX      r1
        0x000000cc:    4605        .F      MOV      r5,r0
        0x000000ce:    6860        `h      LDR      r0,[r4,#4]
        0x000000d0:    2101        .!      MOVS     r1,#1
        0x000000d2:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000d4:    4790        .G      BLX      r2
        0x000000d6:    08a8        ..      LSRS     r0,r5,#2
        0x000000d8:    0080        ..      LSLS     r0,r0,#2
        0x000000da:    b200        ..      SXTH     r0,r0
        0x000000dc:    1080        ..      ASRS     r0,r0,#2
        0x000000de:    f7fffffe    ....    BL       __aeabi_i2f
        0x000000e2:    e8bd4070    ..p@    POP      {r4-r6,lr}
        0x000000e6:    f06f0104    o...    MVN      r1,#4
        0x000000ea:    f7ffbffe    ....    B.W      __ARM_scalbnf
    LM95071_SHOUTDOWN
        0x000000ee:    b510        ..      PUSH     {r4,lr}
        0x000000f0:    4604        .F      MOV      r4,r0
        0x000000f2:    6840        @h      LDR      r0,[r0,#4]
        0x000000f4:    2100        .!      MOVS     r1,#0
        0x000000f6:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000f8:    4790        .G      BLX      r2
        0x000000fa:    6962        bi      LDR      r2,[r4,#0x14]
        0x000000fc:    21ff        .!      MOVS     r1,#0xff
        0x000000fe:    4620         F      MOV      r0,r4
        0x00000100:    4790        .G      BLX      r2
        0x00000102:    6860        `h      LDR      r0,[r4,#4]
        0x00000104:    2101        .!      MOVS     r1,#1
        0x00000106:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000108:    e8bd4010    ...@    POP      {r4,lr}
        0x0000010c:    4710        .G      BX       r2
    LM9507_STR_INIT
        0x0000010e:    f2af01ff    ....    ADR      r1,{pc}-0xfd ; 0x11
        0x00000112:    6101        .a      STR      r1,[r0,#0x10]
        0x00000114:    f2af01e7    ....    ADR      r1,{pc}-0xe3 ; 0x31
        0x00000118:    6141        Aa      STR      r1,[r0,#0x14]
        0x0000011a:    f2af01a3    ....    ADR      r1,{pc}-0xa1 ; 0x79
        0x0000011e:    6181        .a      STR      r1,[r0,#0x18]
        0x00000120:    f2af0169    ..i.    ADR      r1,{pc}-0x65 ; 0xbb
        0x00000124:    61c1        .a      STR      r1,[r0,#0x1c]
        0x00000126:    f2af0139    ..9.    ADR      r1,{pc}-0x37 ; 0xef
        0x0000012a:    6201        .b      STR      r1,[r0,#0x20]
        0x0000012c:    e7fe        ..      B        LM9507_INIT ; 0x10 Section #1

** Section #6 '.rel.text' (SHT_REL)
    Size   : 48 bytes (alignment 4)
    Symbol table #5 '.symtab'
    6 relocations applied to section #1 '.text'


** Section #2 '.arm_vfe_header' (SHT_PROGBITS)
    Size   : 4 bytes (alignment 4)


** Section #3 '.comment' (SHT_PROGBITS)
    Size   : 986 bytes


** Section #4 '.debug_frame' (SHT_PROGBITS)
    Size   : 300 bytes


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
    Size   : 426 bytes


** Section #10 '.ARM.attributes' (SHT_ARM_ATTRIBUTES)
    Size   : 72 bytes


