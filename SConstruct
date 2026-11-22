import os
import subprocess
import sys

import find_gcc

subprocess.run(
    [sys.executable, os.path.join(".vscode", "gen_compile_db.py")],
    check=False,
)

PROJECT = "Led"
BUILD_DIR = "build"
OUTPUT_DIR = os.path.join("Projects", "Output")
LDSCRIPT = os.path.join("Projects", "gcc", "STM32F103C8Tx_FLASH.ld")

FIXED_SOURCES = [
    "Drives/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c",
    "Drives/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s",
    "Projects/gcc/syscalls.c",
]

AUTO_DIRS = [
    "User",
    "Drives/BSP",
    "Drives/SYSTEM",
    "Drives/STM32F1xx_HAL_Driver/Src",
]


def collect_sources(root):
    found = []
    if not os.path.isdir(root):
        return found
    for dirpath, dirnames, filenames in os.walk(root):
        dirnames[:] = [name for name in dirnames if name.lower() != "legacy"]
        for name in filenames:
            lower = name.lower()
            if not lower.endswith((".c", ".s")):
                continue
            if "template" in lower:
                continue
            if "stm32f1xx_hal_driver" in dirpath.replace("\\", "/").lower():
                if not lower.startswith("stm32f1xx_hal"):
                    continue
            found.append(os.path.join(dirpath, name).replace("\\", "/"))
    found.sort()
    return found


SOURCES = list(FIXED_SOURCES)
for folder in AUTO_DIRS:
    SOURCES.extend(collect_sources(folder))

CPPPATH = [
    "User",
    "Drives",
    "Drives/BSP",
    "Drives/CMSIS/Include",
    "Drives/CMSIS/Device/ST/STM32F1xx/Include",
    "Drives/STM32F1xx_HAL_Driver/Inc",
]

CPPDEFINES = [
    "USE_HAL_DRIVER",
    "STM32F103xB",
]

CCFLAGS = [
    "-mcpu=cortex-m3",
    "-mthumb",
    "-mfloat-abi=soft",
    "-std=c99",
    "-Og",
    "-g",
    "-Wall",
    "-ffunction-sections",
    "-fdata-sections",
]

ASFLAGS = [
    "-mcpu=cortex-m3",
    "-mthumb",
    "-x",
    "assembler-with-cpp",
    "-g",
]


gcc_bin = find_gcc.find_gcc_bin()
if not gcc_bin:
    print("找不到 arm-none-eabi-gcc。请把它加入 PATH，或设置环境变量 ARM_GCC_PATH。")
    Exit(1)

gcc = os.path.join(gcc_bin, "arm-none-eabi-gcc.exe")
if not os.path.isfile(gcc):
    gcc = os.path.join(gcc_bin, "arm-none-eabi-gcc")
objcopy = os.path.join(gcc_bin, "arm-none-eabi-objcopy.exe")
if not os.path.isfile(objcopy):
    objcopy = os.path.join(gcc_bin, "arm-none-eabi-objcopy")
size = os.path.join(gcc_bin, "arm-none-eabi-size.exe")
if not os.path.isfile(size):
    size = os.path.join(gcc_bin, "arm-none-eabi-size")

env = Environment(
    ENV=os.environ.copy(),
    tools=[],
    CC=gcc,
    OBJCOPY=objcopy,
    SIZE=size,
    CPPPATH=CPPPATH,
    CPPDEFINES=CPPDEFINES,
    CCFLAGS=CCFLAGS,
    ASFLAGS=ASFLAGS,
    INCPREFIX="-I",
    CPPDEFPREFIX="-D",
    CPPDEFSUFFIX="",
)

linkflags = [
    "-mcpu=cortex-m3",
    "-mthumb",
    "-mfloat-abi=soft",
    "-T" + LDSCRIPT,
    "-Wl,--gc-sections",
    "-Wl,-Map=" + os.path.join(OUTPUT_DIR, PROJECT + ".map") + ",--cref",
    "-Wl,--print-memory-usage",
    "-Wl,--no-warn-rwx-segments",
    "--specs=nano.specs",
    "--specs=nosys.specs",
    "-lc",
    "-lm",
]

objects = []
for src in SOURCES:
    obj = os.path.join(BUILD_DIR, os.path.splitext(src)[0] + ".o")
    if src.endswith(".s"):
        objects.append(env.Command(obj, src, "$CC $ASFLAGS -c -o $TARGET $SOURCE"))
    else:
        objects.append(env.Command(
            obj,
            src,
            "$CC $CCFLAGS $_CPPDEFFLAGS "
            + " ".join("-I" + path for path in CPPPATH)
            + " -c -o $TARGET $SOURCE",
        ))

elf = env.Command(
    os.path.join(OUTPUT_DIR, PROJECT + ".elf"),
    objects,
    "$CC -o $TARGET $SOURCES " + " ".join(linkflags),
)
hexfile = env.Command(
    os.path.join(OUTPUT_DIR, PROJECT + ".hex"),
    elf,
    "$OBJCOPY -O ihex $SOURCE $TARGET",
)
binfile = env.Command(
    os.path.join(OUTPUT_DIR, PROJECT + ".bin"),
    elf,
    "$OBJCOPY -O binary $SOURCE $TARGET",
)
env.AddPostAction(elf, Action("$SIZE $TARGET", "size $TARGET"))

Default(elf, hexfile, binfile)
