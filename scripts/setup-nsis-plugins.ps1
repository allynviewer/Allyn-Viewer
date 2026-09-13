# Installs the NSIS plugins required by indra/newview/installers/windows/installer_template.nsi
# that do not ship with a stock NSIS 3 install:
#   - StdUtils (StdUtils.dll + StdUtils.nsh)  https://github.com/lordmulder/stdutils
#   - INetC    (INetC.dll)                      https://github.com/DigitalMediaServer/NSIS-INetC-plugin
#
# Called by "build.bat install" / "build.bat tools". Safe to run repeatedly: files that
# already exist are left alone. Writing to the NSIS folder under Program Files needs
# administrator rights, so the script re-launches itself elevated when necessary.

[CmdletBinding()]
param(
    [string]$NsisDir = "",
    [switch]$Elevated
)

$ErrorActionPreference = 'Stop'
[Console]::OutputEncoding = [Text.Encoding]::UTF8

$StdUtilsUrl = 'https://github.com/lordmulder/stdutils/releases/download/1.14/StdUtils.2018-10-27.zip'
$InetcUrl    = 'https://github.com/DigitalMediaServer/NSIS-INetC-plugin/releases/download/v1.0.5.7/InetC.zip'

function Find-Nsis {
    param([string]$Hint)
    if ($Hint -and (Test-Path (Join-Path $Hint 'makensis.exe'))) { return $Hint }
    try {
        $reg = (Get-ItemProperty -Path 'HKLM:\SOFTWARE\NSIS' -ErrorAction Stop).'(default)'
        if ($reg -and (Test-Path (Join-Path $reg 'makensis.exe'))) { return $reg }
    } catch { }
    try {
        $reg = (Get-ItemProperty -Path 'HKLM:\SOFTWARE\WOW6432Node\NSIS' -ErrorAction Stop).'(default)'
        if ($reg -and (Test-Path (Join-Path $reg 'makensis.exe'))) { return $reg }
    } catch { }
    foreach ($base in @(${env:ProgramFiles(x86)}, $env:ProgramFiles)) {
        if (-not $base) { continue }
        $cand = Join-Path $base 'NSIS'
        if (Test-Path (Join-Path $cand 'makensis.exe')) { return $cand }
    }
    return $null
}

$nsis = Find-Nsis -Hint $NsisDir
if (-not $nsis) {
    Write-Host '[NSIS] makensis.exe not found - install NSIS first (build.bat tools or https://nsis.sourceforge.io/).'
    exit 2
}

$pluginDir  = Join-Path $nsis 'Plugins\x86-unicode'
$includeDir = Join-Path $nsis 'Include'
$targets = @(
    @{ Path = (Join-Path $pluginDir  'StdUtils.dll'); Zip = 'StdUtils'; Entry = 'Plugins/Unicode/StdUtils.dll' },
    @{ Path = (Join-Path $includeDir 'StdUtils.nsh'); Zip = 'StdUtils'; Entry = 'Include/StdUtils.nsh' },
    @{ Path = (Join-Path $pluginDir  'INetC.dll');    Zip = 'Inetc';    Entry = 'x86-unicode/INetC.dll' }
)

$missing = @($targets | Where-Object { -not (Test-Path $_.Path) })
if ($missing.Count -eq 0) {
    Write-Host "[NSIS] Plugins already installed in $nsis (StdUtils, INetC)."
    exit 0
}

# Can we write to the NSIS folder? If not, re-run elevated.
$canWrite = $true
try {
    $probe = Join-Path $pluginDir ('.allyn_write_test_' + [guid]::NewGuid().ToString('N'))
    New-Item -ItemType File -Path $probe -Force | Out-Null
    Remove-Item $probe -Force
} catch { $canWrite = $false }

if (-not $canWrite) {
    if ($Elevated) {
        Write-Host "[NSIS] Still cannot write to $pluginDir after elevation."
        exit 1
    }
    Write-Host '[NSIS] Administrator rights are required to write into the NSIS folder - requesting elevation (UAC)...'
    $args = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', "`"$PSCommandPath`"", '-NsisDir', "`"$nsis`"", '-Elevated')
    $p = Start-Process -FilePath 'powershell.exe' -ArgumentList $args -Verb RunAs -Wait -PassThru
    exit $p.ExitCode
}

Add-Type -AssemblyName System.IO.Compression.FileSystem
$tmp = Join-Path $env:TEMP ('allyn-nsis-plugins-' + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Path $tmp -Force | Out-Null
try {
    [Net.ServicePointManager]::SecurityProtocol = [Net.ServicePointManager]::SecurityProtocol -bor [Net.SecurityProtocolType]::Tls12
    $zips = @{}
    foreach ($name in ($missing | ForEach-Object { $_.Zip } | Sort-Object -Unique)) {
        $url = if ($name -eq 'StdUtils') { $StdUtilsUrl } else { $InetcUrl }
        $file = Join-Path $tmp "$name.zip"
        Write-Host "[NSIS] Downloading $url"
        Invoke-WebRequest -Uri $url -OutFile $file -UseBasicParsing
        $zips[$name] = [IO.Compression.ZipFile]::OpenRead($file)
    }
    foreach ($t in $missing) {
        $entry = $zips[$t.Zip].Entries | Where-Object { $_.FullName -eq $t.Entry } | Select-Object -First 1
        if (-not $entry) { throw "Entry $($t.Entry) not found in $($t.Zip).zip" }
        $dir = Split-Path $t.Path
        if (-not (Test-Path $dir)) { New-Item -ItemType Directory -Path $dir -Force | Out-Null }
        [IO.Compression.ZipFileExtensions]::ExtractToFile($entry, $t.Path, $true)
        Write-Host "[NSIS] Installed $($t.Path)"
    }
    foreach ($z in $zips.Values) { $z.Dispose() }
    Write-Host "[NSIS] Plugins ready in $nsis."
    exit 0
} finally {
    Remove-Item $tmp -Recurse -Force -ErrorAction SilentlyContinue
}
