#!/usr/bin/env python3
"""Check Keil source paths, and syntax of changed C with real project headers."""
from pathlib import Path
import re
import os
import subprocess
import tempfile
import xml.etree.ElementTree as ET

ROOT = Path(__file__).resolve().parent.parent
PROJECTS = (
    ("STM32L151/MDK-ARM/STM32L151.uvprojx", (
        "Core/Src/main.c", "Software/src/Algorithm_code.c", "Software/src/App_code.c",
        "Software/src/App_key.c", "Software/src/agreement_code.c",
        "Software/src/oled_code.c", "Software/src/usart_code.c", "Software/src/App_Eep.c")),
    ("QINGJIAOBAN/PROJECT/GuiJuChi.uvprojx", (
        "USER/main.c", "USER/stm32f10x_it.c", "CLASS/SCR/lm9507_code.c",
        "CLASS/SCR/usart_code.c", "CLASS/SCR/ads1110_code.c", "CLASS/SCR/iic_code.c", "CLASS/SCR/adc161_code.c")),
)
with tempfile.TemporaryDirectory(prefix="cg34_keil_inc_") as temp:
    alias = Path(temp)
    for project_name, units in PROJECTS:
        project = ROOT / project_name
        tree = ET.parse(project)
        config = tree.find(".//Cads/VariousControls")
        defines = [v for v in re.split(r"[,\s]+", config.findtext("Define")) if v]
        include_dirs = [(project.parent / i.replace("\\", "/")).resolve()
                        for i in config.findtext("IncludePath").split(";") if i]
        for file_node in tree.findall(".//FilePath"):
            path = project.parent / file_node.text.replace("\\", "/")
            if not path.is_file():
                raise SystemExit(f"Missing project input: {path}")
        # Windows Keil's case-insensitive lookup for two legacy include spellings.
        for name in ("Algorithm_head.h", "Circular_head.h"):
            for folder in include_dirs:
                target = folder / name.lower()
                if target.is_file() and not (alias / name).is_file():
                    (alias / name).symlink_to(target)
        options = ["-D" + token for token in defines]
        paths = ["-I" + str(i) for i in include_dirs if i.is_dir()] + ["-I" + temp]
        for entry in units:
            path = project.parent.parent / entry
            subprocess.run([os.environ.get("CC", "cc"), "-std=c99", "-fsyntax-only",
                            "-finput-charset=GBK", "-fexec-charset=GBK", "-Werror=implicit-function-declaration",
                            "-Wno-int-to-pointer-cast", "-Wno-pointer-to-int-cast",
                            *options, *paths, str(path)], check=True)
        print(f"CG34 {project.name}: listed files + {len(units)} C units syntax PASS (not a Keil build)")
