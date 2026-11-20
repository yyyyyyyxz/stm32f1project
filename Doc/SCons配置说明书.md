# SCons 配置说明书

本工程用根目录的 `SConstruct` 调用 ARM GCC，为 STM32F103C8T6 生成 `elf`、`hex`、`bin`。Keil 工程仍保留，两套构建互不影响。

## 环境

需要这三样：

| 工具 | 作用 | 本机位置 |
| --- | --- | --- |
| Python 3 | 运行 SCons | `C:\Users\27443\AppData\Local\Python\pythoncore-3.14-64` |
| SCons | 读 `SConstruct` 并调度编译 | `...\pythoncore-3.14-64\Scripts\scons.exe` |
| ARM GCC | 交叉编译 Cortex-M3 | `D:\App\arm-gnu-toolchain\bin` |

在工程根目录执行：

```powershell
scons -j8
```

Cursor 集成终端的 PATH 由 `.vscode/settings.json` 里的 `terminal.integrated.env.windows.PATH` 整段替换，不继承系统用户 PATH。改完这项设置后要新开一个终端。

## 编译器查找顺序

`find_gcc_bin()` 按下面的顺序找 `arm-none-eabi-gcc.exe`（或无 `.exe` 的同名文件），找到第一个就停止：

1. 环境变量 `ARM_GCC_PATH`（指向 `bin` 目录）
2. 环境变量 `RTT_EXEC_PATH`
3. `D:\App\arm-gnu-toolchain\bin`
4. 当前进程的 `PATH`

工程终端里已经设置了 `ARM_GCC_PATH`，所以会优先用这一处。换电脑或换安装目录时，改环境变量即可，不必改脚本里的固定路径。找不到编译器时脚本会退出，并提示设置 `ARM_GCC_PATH`。

同一目录下还要有 `arm-none-eabi-objcopy` 和 `arm-none-eabi-size`。官方 ARM GCC 和 xPack 工具链都自带这两个程序。

## 常用命令

在含有 `SConstruct` 的目录执行。

| 命令 | 作用 |
| --- | --- |
| `scons` | 编译默认目标：`Led.elf`、`Led.hex`、`Led.bin` |
| `scons -j8` | 最多 8 个任务并行编译 |
| `scons -c` | 删除本次构建产生的目标文件 |
| `scons --dry-run` | 只打印将要执行的命令，不真正编译 |

源文件没有变化时，SCons 会提示目标已是最新。改了 `SConstruct` 里的编译选项后，需要先 `scons -c` 再重新编译，否则旧的 `.o` 不会按新参数重编。

## 产物

| 路径 | 内容 |
| --- | --- |
| `build\` | 各源文件对应的 `.o`，目录结构与源码一致 |
| `Projects\Output\Led.elf` | 带调试信息的可执行文件 |
| `Projects\Output\Led.hex` | Intel HEX，用于烧录 |
| `Projects\Output\Led.bin` | 纯二进制 |
| `Projects\Output\Led.map` | 链接映射，查看符号和段占用 |

链接结束后会打印 Flash / RAM 占用，并再跑一次 `arm-none-eabi-size`。当前链接脚本把 Flash 定为 64 KB（`0x08000000`），RAM 定为 20 KB（`0x20000000`），对应 STM32F103C8。

## 配置项

所有可改项都在 `SConstruct` 顶部。

### 工程名和路径

```python
PROJECT = "Led"
BUILD_DIR = "build"
OUTPUT_DIR = os.path.join("Projects", "Output")
LDSCRIPT = os.path.join("Projects", "gcc", "STM32F103C8Tx_FLASH.ld")
```

`PROJECT` 决定输出文件名，例如改成 `"App"` 后产物变为 `Projects\Output\App.elf`。

### 源文件 `SOURCES`

每个要参与链接的 `.c` 和启动汇编都要写在这里。只被 `#include` 的头文件不要加入。

新增 `Drives/BSP/BEEP/beep.c` 时追加一行：

```python
"Drives/BSP/BEEP/beep.c",
```

启动文件固定为 GCC 版：

`Drives/CMSIS/Device/ST/STM32F1xx/Source/Templates/gcc/startup_stm32f103xb.s`

`Projects/gcc/syscalls.c` 提供 `newlib` 所需的系统调用桩，链接 `--specs=nosys.specs` 时需要它。

### 头文件路径 `CPPPATH`

编译器对每一项加 `-I`。新目录里的头文件找不到时，把该目录加进来，例如：

```python
"Drives/BSP/BEEP",
```

### 宏 `CPPDEFINES`

每一项会变成 `-D名字`。当前有：

- `USE_HAL_DRIVER`：启用 HAL
- `STM32F103xB`：选中 C8 这一档（中容量，`stm32f103xb.h`）

换芯片时要同时改这个宏、启动文件和链接脚本。C8 用 `STM32F103xB` 与 `startup_stm32f103xb.s`。

### 编译选项 `CCFLAGS` 和 `ASFLAGS`

当前面向 Cortex-M3，软浮点，调试优化：

| 选项 | 含义 |
| --- | --- |
| `-mcpu=cortex-m3` | STM32F103 内核 |
| `-mthumb` | Thumb 指令 |
| `-mfloat-abi=soft` | 软件浮点。F103 没有硬件 FPU |
| `-std=c99` | C99 |
| `-Og -g` | 便于调试的优化，并生成调试信息 |
| `-Wall` | 打开常规警告 |
| `-ffunction-sections` `-fdata-sections` | 每个函数、变量单独成段，便于链接时裁剪 |

发布时可以把 `-Og` 改成 `-Os`（体积）或 `-O2`（速度）。汇编选项在 `ASFLAGS`，其中 `-x assembler-with-cpp` 表示启动文件先经过预处理器。

### 链接选项

链接参数写在 `linkflags` 里，不在 `CCFLAGS` 里。

| 选项 | 含义 |
| --- | --- |
| `-TProjects/gcc/STM32F103C8Tx_FLASH.ld` | 链接脚本 |
| `-Wl,--gc-sections` | 丢掉未引用的段 |
| `-Wl,-Map=...` | 生成 `.map` |
| `-Wl,--print-memory-usage` | 打印 Flash / RAM 占用 |
| `--specs=nano.specs` | 使用较小的 newlib nano |
| `--specs=nosys.specs` | 半主机系统调用改为桩函数 |
| `-lc -lm` | 链接 C 库和数学库 |

改 Flash 或 RAM 大小时编辑 `Projects/gcc/STM32F103C8Tx_FLASH.ld` 的 `MEMORY`，不要只改编译选项。

## 换一台电脑时要改的地方

1. 安装 Python，并执行 `python -m pip install scons`。
2. 安装 ARM GNU Toolchain（`arm-none-eabi-gcc`），把 `bin` 目录设为 `ARM_GCC_PATH`。
3. 若使用 Cursor 集成终端，把 SCons 所在的 `Scripts` 目录和 `ARM_GCC_PATH` 写入 `.vscode/settings.json` 的 `terminal.integrated.env.windows`。这里的 `PATH` 会覆盖系统 PATH。

## 常见问题

**提示找不到 `scons`。**  
当前终端的 PATH 里没有 `Scripts`。确认 `scons.exe` 存在后，把它的目录加到 `.vscode/settings.json` 的 `terminal.integrated.env.windows.PATH` 最前面，然后新开终端。也可以直接运行：

```powershell
python -m SCons -j8
```

**提示找不到 `arm-none-eabi-gcc`。**  
设置 `ARM_GCC_PATH` 为工具链的 `bin` 目录，或把该目录加入终端 PATH。脚本不会自动下载编译器。

**改了宏或优化等级，生成文件却没变。**  
先执行 `scons -c`，再执行 `scons -j8`。

**新增了 `.c` 文件但没有参与链接。**  
头文件能包含，不代表源文件会被编译。把 `.c` 路径加入 `SOURCES`。
