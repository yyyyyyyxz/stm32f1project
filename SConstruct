import os

PROJECT = "Led"
BUILD_DIR = "build"
OUTPUT_DIR = os.path.join("Projects", "Output")
LDSCRIPT = os.path.join("Projects", "gcc", "STM32F103C8Tx_FLASH.ld")

SOURCES = [
    "User/main.c",
    "User/init.c",
    "User/stm32f1xx_it.c",
    "Drives/CMSIS/Device/ST/STM32F1xx/Source/Templates/system_stm32f1xx.c",
    "Drives/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s",
    "Drives/BSP/LED/led.c",
    "Drives/BSP/KEY/key.c",
    "Drives/BSP/USART/usart.c",
    "Drives/SYSTEM/delay/delay.c",
    "Drives/SYSTEM/sys/sys.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_cortex.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_dma.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_gpio_ex.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_rcc_ex.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_uart.c",
    "Drives/STM32F1xx_HAL_Driver/Src/stm32f1xx_hal_usart.c",
    "Projects/gcc/syscalls.c",
]

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


def find_gcc_bin():
    names = ("arm-none-eabi-gcc.exe", "arm-none-eabi-gcc")
    candidates = [
        os.environ.get("ARM_GCC_PATH", ""),
        os.environ.get("RTT_EXEC_PATH", ""),
        r"D:\Downloads\App\arm-gnu-toolchain\bin",
    ]
    candidates.extend(os.environ.get("PATH", "").split(os.pathsep))
    for path in candidates:
        if not path:
            continue
        for name in names:
            if os.path.isfile(os.path.join(path, name)):
                return path
    return None


gcc_bin = find_gcc_bin()
if not gcc_bin:
    print("找不到 arm-none-eabi-gcc。请安装 ARM GCC，或设置环境变量 ARM_GCC_PATH。")
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
