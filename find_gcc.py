"""Locate arm-none-eabi-gcc without a machine-specific path in the project."""
import os
import shutil
import string

GCC_NAMES = ("arm-none-eabi-gcc.exe", "arm-none-eabi-gcc")

RELATIVE_BINS = (
    os.path.join("tools", "arm-gnu-toolchain", "bin"),
    os.path.join("arm-gnu-toolchain", "bin"),
)

KNOWN_SUFFIXES = (
    os.path.join("App", "arm-gnu-toolchain", "bin"),
    os.path.join("Downloads", "App", "arm-gnu-toolchain", "bin"),
    os.path.join("App", "env_release", "env-windows", "tools", "gnu_gcc", "arm_gcc", "mingw", "bin"),
)


def project_root():
    return os.path.dirname(os.path.abspath(__file__))


def _directory_of(path, root):
    path = path.strip().strip('"')
    if not path:
        return ""
    if not os.path.isabs(path):
        path = os.path.join(root, path)
    if os.path.isfile(path):
        return os.path.dirname(path)
    return path


def _has_gcc(directory):
    if not directory or not os.path.isdir(directory):
        return False
    return any(os.path.isfile(os.path.join(directory, name)) for name in GCC_NAMES)


def find_gcc_bin(root=None):
    root = root or project_root()
    candidates = []

    for key in ("ARM_GCC_PATH", "RTT_EXEC_PATH"):
        candidates.append(_directory_of(os.environ.get(key, ""), root))

    which = shutil.which("arm-none-eabi-gcc")
    if which:
        candidates.append(os.path.dirname(which))

    for relative in RELATIVE_BINS:
        candidates.append(os.path.join(root, relative))

    candidates.extend(os.environ.get("PATH", "").split(os.pathsep))

    for letter in string.ascii_uppercase:
        drive = letter + ":\\"
        if not os.path.isdir(drive):
            continue
        for suffix in KNOWN_SUFFIXES:
            candidates.append(os.path.join(drive, suffix))

    seen = set()
    for path in candidates:
        if not path:
            continue
        key = os.path.normcase(os.path.normpath(path))
        if key in seen:
            continue
        seen.add(key)
        if _has_gcc(path):
            return os.path.normpath(path)
    return None


def find_gcc_exe(root=None):
    bin_dir = find_gcc_bin(root)
    if not bin_dir:
        return None
    for name in GCC_NAMES:
        exe = os.path.join(bin_dir, name)
        if os.path.isfile(exe):
            return exe
    return None
