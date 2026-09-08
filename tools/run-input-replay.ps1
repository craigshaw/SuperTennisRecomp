[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$RomPath = $env:SUPER_TENNIS_ROM,
    [Parameter(Position = 1)]
    [string]$ReplayPath = $env:SUPER_TENNIS_REPLAY,
    [int]$FrameLimit = 0
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$Headless = Join-Path $Root 'build\windows-msvc-x64-release\super_tennis_headless.exe'

if ([string]::IsNullOrWhiteSpace($RomPath) -or
    -not (Test-Path -LiteralPath $RomPath -PathType Leaf)) {
    throw 'Set -RomPath or SUPER_TENNIS_ROM to the supported user-owned ROM.'
}
if ([string]::IsNullOrWhiteSpace($ReplayPath) -or
    -not (Test-Path -LiteralPath $ReplayPath -PathType Leaf)) {
    throw 'Set -ReplayPath or SUPER_TENNIS_REPLAY to a private .sri replay.'
}
if (-not (Test-Path $Headless -PathType Leaf)) {
    throw 'Missing Windows headless executable. Run build.ps1 first.'
}
if ($FrameLimit -lt 0) {
    throw 'FrameLimit must be zero for the complete replay or a positive frame count.'
}

$arguments = @(
    (Resolve-Path -LiteralPath $RomPath).Path,
    '--replay',
    (Resolve-Path -LiteralPath $ReplayPath).Path
)
if ($FrameLimit -gt 0) {
    $arguments += @('--frames', $FrameLimit.ToString())
}

& $Headless @arguments
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
