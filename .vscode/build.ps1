param(
    [ValidateSet("build", "rebuild", "clean", "flash", "download")]
    [string]$Action = "build"
)

$ErrorActionPreference = "Stop"
$Root = Split-Path -Parent $PSScriptRoot
$Project = Join-Path $Root "Projects\MDK-ARM\Led.uvprojx"
$Target = "Templete"
$LogDir = Join-Path $Root "Projects\Output"
$Hex = Join-Path $LogDir "Led.hex"

$Uv4Candidates = @(
    "D:\App\Keil_v5\UV4\UV4.exe",
    "C:\Keil_v5\UV4\UV4.exe",
    "D:\Keil_v5\UV4\UV4.exe"
)
$Uv4 = $Uv4Candidates | Where-Object { Test-Path $_ } | Select-Object -First 1
if (-not $Uv4) {
    Write-Host "找不到 UV4.exe。请安装 Keil，或修改 .vscode/build.ps1 里的路径。"
    exit 1
}
if (-not (Test-Path $Project)) {
    Write-Host "找不到工程文件: $Project"
    exit 1
}
if (-not (Test-Path $LogDir)) {
    New-Item -ItemType Directory -Path $LogDir | Out-Null
}

function Invoke-Keil {
    param(
        [Parameter(Mandatory = $true)][string]$Flag,
        [Parameter(Mandatory = $true)][string]$Title
    )

    $Log = Join-Path $LogDir "build.log"
    if (Test-Path $Log) {
        Remove-Item $Log -Force
    }

    Write-Host "Keil $Title  $Target"
    Write-Host $Project
    Write-Host ""

    $Process = Start-Process -FilePath $Uv4 -ArgumentList @(
        $Flag, $Project, "-t", $Target, "-j0", "-o", $Log
    ) -Wait -PassThru -WindowStyle Hidden

    if (-not (Test-Path $Log)) {
        Write-Host "Keil 没有生成日志。如果工程已在 Keil 里打开，请先关掉再执行。"
        return 1
    }

    Get-Content -Path $Log -Encoding Default | ForEach-Object { Write-Host $_ }
    Write-Host ""

    $Summary = Select-String -Path $Log -Pattern "(\d+)\s+Error\(s\),\s+(\d+)\s+Warning\(s\)" |
        Select-Object -Last 1
    if ($Summary) {
        $Script:KeilErrors = [int]$Summary.Matches[0].Groups[1].Value
        $Script:KeilWarnings = [int]$Summary.Matches[0].Groups[2].Value
        if ($Script:KeilErrors -gt 0) {
            return 1
        }
        return 0
    }

    $Script:KeilErrors = 0
    $Script:KeilWarnings = 0
    $Failed = Select-String -Path $Log -Pattern "Flash Download failed|No ST-LINK|No ULINK" -Quiet
    if ($Failed -or $Process.ExitCode -ge 2) {
        $Script:KeilErrors = 1
        return 1
    }
    return 0
}

function Invoke-OpenOcd {
    $OpenOcdCandidates = @(
        "D:\App\Openocd\bin\openocd.exe",
        "D:\App\OpenOCD\bin\openocd.exe"
    )
    $OpenOcd = $OpenOcdCandidates | Where-Object { Test-Path $_ } | Select-Object -First 1
    if (-not $OpenOcd) {
        $Cmd = Get-Command openocd -ErrorAction SilentlyContinue
        if ($Cmd) { $OpenOcd = $Cmd.Source }
    }
    if (-not $OpenOcd) {
        Write-Host "找不到 openocd.exe。"
        return 1
    }

    $HexTcl = ($Hex -replace '\\', '/')
    $Interface = "interface/stlink.cfg"
    $ProbeFile = Join-Path $PSScriptRoot "probe.txt"
    if (Test-Path $ProbeFile) {
        $Probe = (Get-Content $ProbeFile -Raw).Trim()
        if ($Probe) { $Interface = $Probe }
    }

    Write-Host "openocd -f $Interface -f board/stm32f103c8_blue_pill.cfg -c ""program $HexTcl verify reset exit"""
    Write-Host ""

    $Process = Start-Process -FilePath $OpenOcd -WorkingDirectory $Root -ArgumentList @(
        "-f", $Interface,
        "-f", "board/stm32f103c8_blue_pill.cfg",
        "-c", "program {$HexTcl} verify reset exit"
    ) -Wait -PassThru -NoNewWindow

    if ($Process.ExitCode -ne 0) {
        return 1
    }
    return 0
}

switch ($Action) {
    "rebuild" {
        $Status = Invoke-Keil "-r" "重新编译"
        if ($Status -eq 0 -and (Test-Path $Hex)) {
            Write-Host "重新编译成功: $Hex"
            exit 0
        }
        Write-Host "重新编译失败: $KeilErrors 个错误, $KeilWarnings 个警告"
        exit 1
    }
    "clean" {
        $Status = Invoke-Keil "-c" "清理"
        if ($Status -ne 0) {
            Write-Host "清理失败"
            exit 1
        }
        Write-Host "清理完成"
        exit 0
    }
    "flash" {
        if (-not (Test-Path $Hex)) {
            Write-Host "没有找到 $Hex，请先编译。"
            exit 1
        }
        $Status = Invoke-OpenOcd
        if ($Status -ne 0) {
            Write-Host "下载失败"
            exit 1
        }
        Write-Host "下载完成: $Hex"
        exit 0
    }
    "download" {
        $Status = Invoke-Keil "-b" "编译"
        if ($Status -ne 0 -or -not (Test-Path $Hex)) {
            Write-Host "编译失败: $KeilErrors 个错误, $KeilWarnings 个警告"
            exit 1
        }
        Write-Host "编译成功: $Hex"
        Write-Host ""
        $Status = Invoke-OpenOcd
        if ($Status -ne 0) {
            Write-Host "下载失败"
            exit 1
        }
        Write-Host "下载完成: $Hex"
        exit 0
    }
    default {
        $Status = Invoke-Keil "-b" "编译"
        if ($Status -eq 0 -and (Test-Path $Hex)) {
            Write-Host "编译成功: $Hex"
            if ($KeilWarnings -gt 0) {
                Write-Host "警告: $KeilWarnings"
            }
            exit 0
        }
        Write-Host "编译失败: $KeilErrors 个错误, $KeilWarnings 个警告"
        exit 1
    }
}
