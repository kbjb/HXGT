"""Exercise actual ADC161 packing and signed decoding, independent of SPI timing."""
from pathlib import Path
import os
import re
import subprocess
import tempfile

root = Path(__file__).resolve().parents[2]
adc = (root/'QINGJIAOBAN/CLASS/SCR/adc161_code.c').read_text(encoding='gb18030')
signed = re.search(r'int16_t uint16_twos_complement_to_int16_original[\s\S]+?\n}', adc).group()
decode = re.search(r'bits = .+?;', adc).group()
code = """
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
""" + signed + """
static int16_t read_back(uint16_t first, uint16_t second)
{
    uint16_t bits;
    """ + decode + """
    return uint16_twos_complement_to_int16_original(bits);
}
int main(void)
{
    int32_t i;
    for (i=-32768;i<=32767;i++)
    {
        uint16_t raw = (uint16_t)i;
        uint16_t first = (uint16_t)((raw >> 2) | 0xC000U);
        uint16_t second = (uint16_t)(((raw & 3U) << 14) | 0x3FFFU);
        assert(read_back(first, second)==i);
    }
    puts("CG41 SPI ADC: PASS all 65536 signed codes with dummy/extra clocks");
}
"""
with tempfile.TemporaryDirectory(prefix='cg41_spi_') as temp:
    source = Path(temp)/'adc.c'
    exe = Path(temp)/('adc.exe' if os.name=='nt' else 'adc')
    source.write_text(code, encoding='ascii')
    cmd=[os.environ.get('CC','cc'),'-std=c99','-Wall','-Wextra','-Werror']
    if os.environ.get('HOST_SANITIZERS','1')!='0':
        cmd+=['-fsanitize=undefined']
    subprocess.run(cmd+[str(source),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
