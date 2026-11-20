#!/usr/bin/env python3
"""Read-only AT24C512 dump inspection utility.

Usage:
  python3 B3_HOST_TESTS/inspect_dump.py eeprom_64k.bin
  python3 B3_HOST_TESTS/inspect_dump.py eeprom_64k.bin --json report.json

No writing to EEPROM, no connection to Keil, no automatic data migration.
The caller must acquire an external programmer's verified 65,536-byte dump.
"""
from __future__ import annotations
import argparse
import hashlib
import json
from pathlib import Path

EEPROM_LEN=65536
CAL_SLOTS=(0xE000,0xE080)
REC_SLOTS=(0xE100,0xE180)

def u16(b: bytes) -> int:
    return (b[0]<<8)|b[1]

def u32(b: bytes) -> int:
    return (b[0]<<24)|(b[1]<<16)|(b[2]<<8)|b[3]

def crc16(data: bytes) -> int:
    c=0xFFFF
    for x in data:
        c^=x<<8
        for _ in range(8):
            c=((c<<1)^0x1021)&0xFFFF if c&0x8000 else (c<<1)&0xFFFF
    return c

def describe_slot(data: bytes, addr:int, magic:bytes, kind:str)->dict:
    p=data[addr:addr+64]
    blank=all(x==0xFF for x in p)
    valid=(len(p)==64 and p[:4]==magic and p[4]==3 and p[5]==0
           and p[63]==0xA5 and crc16(p[:61])==u16(p[61:63]))
    if valid and kind=="record":
        valid=u16(p[10:12])<=100
    obj={"address":f"0x{addr:04X}","state":"BLANK" if blank else "VALID" if valid else "INVALID"}
    if valid:
        obj["sequence"]=u32(p[6:10])
        if kind=="record":obj["record_count"]=u16(p[10:12])
        else:
            obj["cg_adc"]= [u16(p[x:x+2]) for x in (14,16,18,20)]
            obj["gj_adc"]= [u16(p[x:x+2]) for x in (26,28,30)]
            obj["ch_adc"]= [u16(p[x:x+2]) for x in (36,38)]
            obj["password"]="[redacted]"
    return obj

def main()->None:
    parser=argparse.ArgumentParser(description=__doc__)
    parser.add_argument("dump",type=Path,help="Raw 65536-byte AT24C512 image")
    parser.add_argument("--json",type=Path,default=None,help="Output report path")
    args=parser.parse_args()
    payload=args.dump.read_bytes()
    if len(payload)!=EEPROM_LEN:
        parser.error(f"Expected exactly 65536 bytes, got {len(payload)}")
    result={
        "format":"AT24C512 raw dump, no writes",
        "sha256":hashlib.sha256(payload).hexdigest(),
        "size":len(payload),
        "b3_calibration":[describe_slot(payload,a,b"GJB3","cal") for a in CAL_SLOTS],
        "b3_record_metadata":[describe_slot(payload,a,b"RCB3","record") for a in REC_SLOTS],
        "legacy":{
            "flag_at_FFFF":f"0x{payload[0xFFFF]:02X}",
            "password":"[redacted]",
            "cg_adc_hex":{
                f"0x{a:04X}":payload[a:a+2].hex().upper() for a in (65529,65527,65525,65523)
            },
            "gj_adc_hex":{
                f"0x{a:04X}":payload[a:a+2].hex().upper() for a in (65517,65515,65513)
            },
            "ch_adc_hex":{
                f"0x{a:04X}":payload[a:a+2].hex().upper() for a in (65507,65505)
            },
            "overlapped_legacy_index_raw":u16(payload[65503:65505]),
            "overlapped_legacy_count_raw":u16(payload[65505:65507]),
            "warning":"Legacy counters overlap CH calibration; legacy float writes may corrupt adjacent ADC bytes. The counters and temperatures must NOT be trusted without external verification."
        }
    }
    rendered=json.dumps(result,ensure_ascii=False,indent=2)
    print(rendered)
    if args.json:
        args.json.write_text(rendered+"\n",encoding="utf-8")
if __name__=="__main__":
    main()
