
========================================================================

** ELF Header Information

    File Name: C:\Users\zhangqingqing\Documents\ChatGPT\SXGJC 2\tmp\cg40_work\delay_after_O0.o

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
    Section header offset: 2656 (0x00000a60)

    Section header string table index: 8

========================================================================

** Section #1 '.text' (SHT_PROGBITS) [SHF_ALLOC + SHF_EXECINSTR]
    Size   : 318 bytes (alignment 4)
    Address: 0x00000000

    $t
    .text
    LM9507_DELAY
        0x00000000:    b508        ..      PUSH     {r3,lr}
        0x00000002:    9000        ..      STR      r0,[sp,#0]
        0x00000004:    e003        ..      B        {pc}+0xa ; 0xe
        0x00000006:    bf00        ..      NOP      
        0x00000008:    9900        ..      LDR      r1,[sp,#0]
        0x0000000a:    1e49        I.      SUBS     r1,r1,#1
        0x0000000c:    9100        ..      STR      r1,[sp,#0]
        0x0000000e:    9900        ..      LDR      r1,[sp,#0]
        0x00000010:    2900        .)      CMP      r1,#0
        0x00000012:    d1f8        ..      BNE      {pc}-0xc ; 0x6
        0x00000014:    bd08        ..      POP      {r3,pc}
    LM9507_INIT
        0x00000016:    b510        ..      PUSH     {r4,lr}
        0x00000018:    4604        .F      MOV      r4,r0
        0x0000001a:    6820         h      LDR      r0,[r4,#0]
        0x0000001c:    68c1        .h      LDR      r1,[r0,#0xc]
        0x0000001e:    4788        .G      BLX      r1
        0x00000020:    6860        `h      LDR      r0,[r4,#4]
        0x00000022:    68c1        .h      LDR      r1,[r0,#0xc]
        0x00000024:    4788        .G      BLX      r1
        0x00000026:    68a0        .h      LDR      r0,[r4,#8]
        0x00000028:    68c1        .h      LDR      r1,[r0,#0xc]
        0x0000002a:    4788        .G      BLX      r1
        0x0000002c:    68e0        .h      LDR      r0,[r4,#0xc]
        0x0000002e:    68c1        .h      LDR      r1,[r0,#0xc]
        0x00000030:    4788        .G      BLX      r1
        0x00000032:    bd10        ..      POP      {r4,pc}
    LM9507_WRITE
        0x00000034:    b570        p.      PUSH     {r4-r6,lr}
        0x00000036:    4604        .F      MOV      r4,r0
        0x00000038:    460e        .F      MOV      r6,r1
        0x0000003a:    250f        .%      MOVS     r5,#0xf
        0x0000003c:    e01a        ..      B        {pc}+0x38 ; 0x74
        0x0000003e:    2301        .#      MOVS     r3,#1
        0x00000040:    40ab        .@      LSLS     r3,r3,r5
        0x00000042:    4033        3@      ANDS     r3,r3,r6
        0x00000044:    b10b        ..      CBZ      r3,{pc}+0x6 ; 0x4a
        0x00000046:    2301        .#      MOVS     r3,#1
        0x00000048:    e000        ..      B        {pc}+0x4 ; 0x4c
        0x0000004a:    2300        .#      MOVS     r3,#0
        0x0000004c:    4619        .F      MOV      r1,r3
        0x0000004e:    68a0        .h      LDR      r0,[r4,#8]
        0x00000050:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000052:    4790        .G      BLX      r2
        0x00000054:    6820         h      LDR      r0,[r4,#0]
        0x00000056:    2101        .!      MOVS     r1,#1
        0x00000058:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x0000005a:    4790        .G      BLX      r2
        0x0000005c:    200a        .       MOVS     r0,#0xa
        0x0000005e:    f7fffffe    ....    BL       LM9507_DELAY ; 0x0 Section #1
        0x00000062:    6820         h      LDR      r0,[r4,#0]
        0x00000064:    2100        .!      MOVS     r1,#0
        0x00000066:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000068:    4790        .G      BLX      r2
        0x0000006a:    200a        .       MOVS     r0,#0xa
        0x0000006c:    f7fffffe    ....    BL       LM9507_DELAY ; 0x0 Section #1
        0x00000070:    1e68        h.      SUBS     r0,r5,#1
        0x00000072:    b205        ..      SXTH     r5,r0
        0x00000074:    2d00        .-      CMP      r5,#0
        0x00000076:    dae2        ..      BGE      {pc}-0x38 ; 0x3e
        0x00000078:    bd70        p.      POP      {r4-r6,pc}
    LM9507_READ
        0x0000007a:    b570        p.      PUSH     {r4-r6,lr}
        0x0000007c:    4604        .F      MOV      r4,r0
        0x0000007e:    2500        .%      MOVS     r5,#0
        0x00000080:    2600        .&      MOVS     r6,#0
        0x00000082:    e017        ..      B        {pc}+0x32 ; 0xb4
        0x00000084:    6820         h      LDR      r0,[r4,#0]
        0x00000086:    2101        .!      MOVS     r1,#1
        0x00000088:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x0000008a:    4790        .G      BLX      r2
        0x0000008c:    f64f70ff    O..p    MOV      r0,#0xffff
        0x00000090:    ea000545    ..E.    AND      r5,r0,r5,LSL #1
        0x00000094:    68e0        .h      LDR      r0,[r4,#0xc]
        0x00000096:    6a01        .j      LDR      r1,[r0,#0x20]
        0x00000098:    4788        .G      BLX      r1
        0x0000009a:    2801        .(      CMP      r0,#1
        0x0000009c:    d101        ..      BNE      {pc}+0x6 ; 0xa2
        0x0000009e:    1c68        h.      ADDS     r0,r5,#1
        0x000000a0:    b285        ..      UXTH     r5,r0
        0x000000a2:    6820         h      LDR      r0,[r4,#0]
        0x000000a4:    2100        .!      MOVS     r1,#0
        0x000000a6:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000a8:    4790        .G      BLX      r2
        0x000000aa:    200a        .       MOVS     r0,#0xa
        0x000000ac:    f7fffffe    ....    BL       LM9507_DELAY ; 0x0 Section #1
        0x000000b0:    1c70        p.      ADDS     r0,r6,#1
        0x000000b2:    b2c6        ..      UXTB     r6,r0
        0x000000b4:    2e10        ..      CMP      r6,#0x10
        0x000000b6:    dbe5        ..      BLT      {pc}-0x32 ; 0x84
        0x000000b8:    4628        (F      MOV      r0,r5
        0x000000ba:    bd70        p.      POP      {r4-r6,pc}
    LM95071_READTEMP
        0x000000bc:    e92d41f0    -..A    PUSH     {r4-r8,lr}
        0x000000c0:    4604        .F      MOV      r4,r0
        0x000000c2:    6860        `h      LDR      r0,[r4,#4]
        0x000000c4:    2100        .!      MOVS     r1,#0
        0x000000c6:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000c8:    4790        .G      BLX      r2
        0x000000ca:    4620         F      MOV      r0,r4
        0x000000cc:    69a1        .i      LDR      r1,[r4,#0x18]
        0x000000ce:    4788        .G      BLX      r1
        0x000000d0:    4605        .F      MOV      r5,r0
        0x000000d2:    6860        `h      LDR      r0,[r4,#4]
        0x000000d4:    2101        .!      MOVS     r1,#1
        0x000000d6:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x000000d8:    4790        .G      BLX      r2
        0x000000da:    10ad        ..      ASRS     r5,r5,#2
        0x000000dc:    00a8        ..      LSLS     r0,r5,#2
        0x000000de:    b206        ..      SXTH     r6,r0
        0x000000e0:    10b6        ..      ASRS     r6,r6,#2
        0x000000e2:    4630        0F      MOV      r0,r6
        0x000000e4:    f7fffffe    ....    BL       __aeabi_i2f
        0x000000e8:    4607        .F      MOV      r7,r0
        0x000000ea:    f04f5174    O.tQ    MOV      r1,#0x3d000000
        0x000000ee:    f7fffffe    ....    BL       __aeabi_fmul
        0x000000f2:    e8bd81f0    ....    POP      {r4-r8,pc}
    LM95071_SHOUTDOWN
        0x000000f6:    b510        ..      PUSH     {r4,lr}
        0x000000f8:    4604        .F      MOV      r4,r0
        0x000000fa:    6860        `h      LDR      r0,[r4,#4]
        0x000000fc:    2100        .!      MOVS     r1,#0
        0x000000fe:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000100:    4790        .G      BLX      r2
        0x00000102:    21ff        .!      MOVS     r1,#0xff
        0x00000104:    4620         F      MOV      r0,r4
        0x00000106:    6962        bi      LDR      r2,[r4,#0x14]
        0x00000108:    4790        .G      BLX      r2
        0x0000010a:    6860        `h      LDR      r0,[r4,#4]
        0x0000010c:    2101        .!      MOVS     r1,#1
        0x0000010e:    69c2        .i      LDR      r2,[r0,#0x1c]
        0x00000110:    4790        .G      BLX      r2
        0x00000112:    bd10        ..      POP      {r4,pc}
    LM9507_STR_INIT
        0x00000114:    b510        ..      PUSH     {r4,lr}
        0x00000116:    4604        .F      MOV      r4,r0
        0x00000118:    f2af1005    ....    ADR      r0,{pc}-0x101 ; 0x17
        0x0000011c:    6120         a      STR      r0,[r4,#0x10]
        0x0000011e:    f2af00eb    ....    ADR      r0,{pc}-0xe9 ; 0x35
        0x00000122:    6160        `a      STR      r0,[r4,#0x14]
        0x00000124:    f2af00ad    ....    ADR      r0,{pc}-0xa9 ; 0x7b
        0x00000128:    61a0        .a      STR      r0,[r4,#0x18]
        0x0000012a:    f2af006f    ..o.    ADR      r0,{pc}-0x6d ; 0xbd
        0x0000012e:    61e0        .a      STR      r0,[r4,#0x1c]
        0x00000130:    f2af003d    ..=.    ADR      r0,{pc}-0x39 ; 0xf7
        0x00000134:    6220         b      STR      r0,[r4,#0x20]
        0x00000136:    4620         F      MOV      r0,r4
        0x00000138:    6921        !i      LDR      r1,[r4,#0x10]
        0x0000013a:    4788        .G      BLX      r1
        0x0000013c:    bd10        ..      POP      {r4,pc}

** Section #6 '.rel.text' (SHT_REL)
    Size   : 40 bytes (alignment 4)
    Symbol table #5 '.symtab'
    5 relocations applied to section #1 '.text'


** Section #2 '.arm_vfe_header' (SHT_PROGBITS)
    Size   : 4 bytes (alignment 4)


** Section #3 '.comment' (SHT_PROGBITS)
    Size   : 990 bytes


** Section #4 '.debug_frame' (SHT_PROGBITS)
    Size   : 288 bytes


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
    Size   : 425 bytes


** Section #10 '.ARM.attributes' (SHT_ARM_ATTRIBUTES)
    Size   : 72 bytes


