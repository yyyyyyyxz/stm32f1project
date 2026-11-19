$Build = Join-Path $PSScriptRoot "build.ps1"
$Root = Split-Path -Parent $PSScriptRoot
$ProbeFile = Join-Path $PSScriptRoot "probe.txt"
$Items = @(
    @{ Name = "编译";       Action = "build" },
    @{ Name = "重新编译";   Action = "rebuild" },
    @{ Name = "清理";       Action = "clean" },
    @{ Name = "下载";       Action = "flash" },
    @{ Name = "编译并下载"; Action = "download" },
    @{ Name = "仿真器";     Action = "probe" },
    @{ Name = "输入命令";   Action = "command" }
)
$Probes = @(
    @{ Name = "ST-Link";    File = "interface/stlink.cfg" },
    @{ Name = "CMSIS-DAP";  File = "interface/cmsis-dap.cfg" },
    @{ Name = "J-Link";     File = "interface/jlink.cfg" }
)

function Get-Probe {
    if (Test-Path $ProbeFile) {
        $Line = (Get-Content $ProbeFile -Raw).Trim()
        $Found = $Probes | Where-Object { $_.File -eq $Line } | Select-Object -First 1
        if ($Found) { return $Found }
    }
    return $Probes[0]
}

function Show-Menu {
    param([int]$Selected)
    $Probe = Get-Probe
    Clear-Host
    Write-Host "编译命令    上下选择，回车执行，Esc 退出"
    Write-Host ("仿真器    " + $Probe.Name)
    Write-Host ""
    for ($i = 0; $i -lt $Items.Count; $i++) {
        $Label = $Items[$i].Name
        if ($Items[$i].Action -eq "probe") {
            $Label = "仿真器：" + $Probe.Name
        }
        if ($i -eq $Selected) {
            Write-Host ("> " + $Label) -ForegroundColor Cyan
        } else {
            Write-Host ("  " + $Label)
        }
    }
}

function Select-Probe {
    $Current = Get-Probe
    $Index = 0
    for ($i = 0; $i -lt $Probes.Count; $i++) {
        if ($Probes[$i].File -eq $Current.File) { $Index = $i }
    }
    while ($true) {
        Clear-Host
        Write-Host "选择仿真器    上下选择，回车确认，Esc 返回"
        Write-Host ""
        for ($i = 0; $i -lt $Probes.Count; $i++) {
            if ($i -eq $Index) {
                Write-Host ("> " + $Probes[$i].Name) -ForegroundColor Cyan
            } else {
                Write-Host ("  " + $Probes[$i].Name)
            }
        }
        $Key = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
        switch ($Key.VirtualKeyCode) {
            38 { if ($Index -gt 0) { $Index-- } }
            40 { if ($Index -lt ($Probes.Count - 1)) { $Index++ } }
            13 {
                Set-Content -Path $ProbeFile -Value $Probes[$Index].File -Encoding ascii
                return
            }
            27 { return }
        }
    }
}

function Enter-Command {
    $Probe = Get-Probe
    $Hex = "Projects/Output/Led.hex"
    Clear-Host
    Write-Host "输入命令，空行返回菜单。当前仿真器：" $Probe.Name
    Write-Host ""
    Write-Host "烧录示例："
    Write-Host ("openocd -f " + $Probe.File + " -f board/stm32f103c8_blue_pill.cfg -c ""program " + $Hex + " verify reset exit""")
    Write-Host ""
    while ($true) {
        $Line = Read-Host ">"
        if (-not $Line) { return }
        Push-Location $Root
        try {
            cmd.exe /c $Line
        } finally {
            Pop-Location
        }
        Write-Host ""
    }
}

$Index = 0
while ($true) {
    Show-Menu $Index
    $Key = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
    switch ($Key.VirtualKeyCode) {
        38 { if ($Index -gt 0) { $Index-- } }
        40 { if ($Index -lt ($Items.Count - 1)) { $Index++ } }
        13 {
            $Action = $Items[$Index].Action
            if ($Action -eq "probe") {
                Select-Probe
            } elseif ($Action -eq "command") {
                Enter-Command
            } else {
                Clear-Host
                & powershell.exe -NoProfile -ExecutionPolicy Bypass -File $Build $Action
                Write-Host ""
                Write-Host "按任意键返回菜单"
                $null = $Host.UI.RawUI.ReadKey("NoEcho,IncludeKeyDown")
            }
        }
        27 { exit 0 }
    }
}
