"""Write .vscode/compile_commands.json for the current project."""
import json
import os
import sys

ROOT = os.path.abspath(os.path.join(os.path.dirname(__file__), ".."))
sys.path.insert(0, ROOT)

import find_gcc

GCC = find_gcc.find_gcc_exe(ROOT)

INCLUDES = [
    ROOT,
    os.path.join(ROOT, "User"),
    os.path.join(ROOT, "Drives"),
    os.path.join(ROOT, "Drives", "CMSIS", "Include"),
    os.path.join(ROOT, "Drives", "CMSIS", "Device", "ST", "STM32F1xx", "Include"),
    os.path.join(ROOT, "Drives", "STM32F1xx_HAL_Driver", "Inc"),
]

SOURCE_DIRS = [
    os.path.join(ROOT, "User"),
    os.path.join(ROOT, "Drives", "BSP"),
    os.path.join(ROOT, "Drives", "SYSTEM"),
    os.path.join(ROOT, "Drives", "STM32F1xx_HAL_Driver", "Src"),
    os.path.join(ROOT, "Projects", "gcc"),
]


def to_posix(path):
    return path.replace("\\", "/")


def collect_sources():
    sources = []
    for folder in SOURCE_DIRS:
        if not os.path.isdir(folder):
            continue
        for dirpath, dirnames, filenames in os.walk(folder):
            dirnames[:] = [name for name in dirnames if name.lower() != "legacy"]
            for name in filenames:
                lower = name.lower()
                if not lower.endswith(".c"):
                    continue
                if "template" in lower:
                    continue
                sources.append(os.path.join(dirpath, name))
    sources.sort()
    return sources


def main():
    if not GCC:
        print("找不到 arm-none-eabi-gcc。请把它加入 PATH，或设置环境变量 ARM_GCC_PATH。")
        return 1

    flags = [
        to_posix(GCC),
        "-mcpu=cortex-m3",
        "-mthumb",
        "-mfloat-abi=soft",
        "-std=c99",
        "-DUSE_HAL_DRIVER",
        "-DSTM32F103xB",
        "-D__STDC_LIMIT_MACROS",
    ]
    for include in INCLUDES:
        flags.append("-I" + to_posix(include))

    entries = []
    root = to_posix(ROOT)
    for source in collect_sources():
        source_path = to_posix(source)
        entries.append({
            "directory": root,
            "file": source_path,
            "arguments": flags + ["-c", source_path],
        })

    output = os.path.join(ROOT, ".vscode", "compile_commands.json")
    with open(output, "w", encoding="utf-8", newline="\n") as handle:
        json.dump(entries, handle, indent=4)
        handle.write("\n")
    print("compile_commands.json: %d files" % len(entries))
    return 0


if __name__ == "__main__":
    sys.exit(main())
