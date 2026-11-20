# SCons 工具链说明书

这份说明只讲怎么把 SCons 和 ARM GCC 装好，并让终端找得到它们。工程里改源文件、宏、优化等级，看同目录的《SCons配置说明书》。

工具链分成两截：

| 部分 | 程序 | 作用 |
| --- | --- | --- |
| 构建调度 | `scons.exe` | 读取当前目录的 `SConstruct`，决定编译哪些文件 |
| 交叉编译 | `arm-none-eabi-gcc` | 把 C 和汇编编成 Cortex-M3 的目标文件并链接 |
| 交叉编译 | `arm-none-eabi-objcopy` | 从 `elf` 生成 `hex` 和 `bin` |
| 交叉编译 | `arm-none-eabi-size` | 打印 Flash / RAM 占用 |

`scons` 自己不会去下载编译器。当前目录没有 `SConstruct` 时，它也不知道要编什么。

## 本机已经放好的位置

| 内容 | 路径 |
| --- | --- |
| SCons | `C:\Users\27443\AppData\Local\Python\pythoncore-3.14-64\Scripts\scons.exe` |
| SCons 的 cmd 包装 | `C:\Users\27443\AppData\Local\Python\bin\scons.cmd` |
| 单独安装的 ARM GCC 15.2.1 | `D:\Downloads\App\arm-gnu-toolchain\bin` |
| RT-Thread Env | `D:\App\env_release\env-windows` |
| Env 自带的 ARM GCC | `D:\App\env_release\env-windows\tools\gnu_gcc\arm_gcc\mingw\bin` |
| Env 虚拟环境里的 SCons | `D:\App\env_release\env-windows\.venv\Scripts` |

本工程现在用的是单独安装的那套 ARM GCC。`SConstruct` 先看 `ARM_GCC_PATH`，工程终端里已经把它设成 `D:\Downloads\App\arm-gnu-toolchain\bin`。

## 做法一：用 RT-Thread Env 终端

Cursor 用户设置里有两个终端配置，选中后会先跑 Env 脚本，再进入命令行：

| 配置名 | 启动方式 |
| --- | --- |
| Env | `cmd.exe /K D:\App\env_release\env-windows\vscode-env.cmd` |
| Env (PowerShell) | `powershell.exe -File D:\App\env_release\env-windows\vscode-env.ps1` |

`vscode-env.ps1` 做三件事：

1. 把 `RTT_EXEC_PATH` 设为 Env 自带的 `arm_gcc\mingw\bin`。
2. 把 `.venv\Scripts`、`tools\bin`、这套 GCC、Env 自带 Git 接到当前 `PATH` 最前面。
3. 激活 `.venv`。提示符变成 `ENV 当前目录>`，并提示可以直接运行 `scons`。

因此在 Env 终端里，`scons` 来自 Env 的虚拟环境，不依赖系统用户 PATH。`SConstruct` 若没有 `ARM_GCC_PATH`，下一步就会使用这里的 `RTT_EXEC_PATH`。

脚本里的 `$EnvRoot` 必须指向真实的 `env-windows` 目录。指错时，这个终端里的 `scons` 和 `arm-none-eabi-gcc` 都会找不到。

## 做法二：单独安装

不打开 Env 终端时，按下面两步安装。

### 安装 SCons

已有 Python 3 时：

```powershell
python -m pip install scons
```

装完后的程序在 Python 的 `Scripts` 目录。确认：

```powershell
python -m SCons --version
```

`python -m SCons` 不依赖 PATH。要直接输入 `scons`，`Scripts` 目录必须出现在当前终端的 PATH 里。本机这个目录是：

`C:\Users\27443\AppData\Local\Python\pythoncore-3.14-64\Scripts`

用户环境变量 PATH 里已经包含它。已经打开的终端不会自动更新，新开一个终端才会读到。

### 安装 ARM GCC

需要带 `arm-none-eabi-gcc`、`arm-none-eabi-objcopy`、`arm-none-eabi-size` 的 Windows 工具链。本机使用的是 xPack GNU Arm Embedded GCC 15.2.1，解压后的 `bin` 放在：

`D:\Downloads\App\arm-gnu-toolchain\bin`

换目录时保持这个结构：`某个目录\bin\arm-none-eabi-gcc.exe`。然后把该 `bin` 目录设为环境变量 `ARM_GCC_PATH`。`SConstruct` 认的是目录，不是 `gcc.exe` 的完整路径。

确认：

```powershell
& "$env:ARM_GCC_PATH\arm-none-eabi-gcc.exe" --version
```

## 让 Cursor 工程终端找得到

本工程 `.vscode/settings.json` 里的 `terminal.integrated.env.windows` 会替换集成终端的整个 PATH，不继承系统用户 PATH。只改 Windows 的用户 PATH，这个工程的终端仍然可能找不到 `scons`。

这项里要同时有：

- `PATH` 最前面带上 `Scripts` 目录和 `C:\Users\27443\AppData\Local\Python\bin`
- `ARM_GCC_PATH` 指向 `D:\Downloads\App\arm-gnu-toolchain\bin`

改完后关掉当前终端，新开一个。已经存在的终端继续用旧环境。

Env 终端是后执行 `vscode-env.ps1` 的，它会在这个 PATH 前面再接上 Env 自己的目录，所以 Env 终端里两种 `scons` 都能被找到，排在前面的是 Env 虚拟环境那一份。

## 本工程选中哪套 GCC

`SConstruct` 的 `find_gcc_bin()` 按顺序找 `arm-none-eabi-gcc.exe`，找到就停止：

1. `ARM_GCC_PATH`
2. `RTT_EXEC_PATH`
3. `D:\Downloads\App\arm-gnu-toolchain\bin`
4. 当前进程的 PATH

工程终端设置了 `ARM_GCC_PATH` 时，用的是单独安装的 15.2.1，不会用到 Env 自带的 GCC。要改回 Env 自带的编译器，去掉 `ARM_GCC_PATH`，保留 `RTT_EXEC_PATH`。

同一目录里还必须有 `arm-none-eabi-objcopy` 和 `arm-none-eabi-size`，否则编译能过，生成 `hex` 或打印大小时会失败。

## 检查

在工程根目录、新开的终端里执行：

```powershell
scons --version
arm-none-eabi-gcc --version
scons -j8
```

三条都成功时，工具链可用。`scons -j8` 会读取本目录的 `SConstruct` 并编译出 `Projects\Output\Led.elf`、`Led.hex`、`Led.bin`。

只想确认编译器被哪一处选中，看 `scons -j8` 打印的第一条命令。它应类似：

```text
D:\Downloads\App\arm-gnu-toolchain\bin\arm-none-eabi-gcc.exe -mcpu=cortex-m3 ...
```

路径若是 `env-windows\tools\gnu_gcc\...`，说明当前用的是 Env 自带的 GCC。
