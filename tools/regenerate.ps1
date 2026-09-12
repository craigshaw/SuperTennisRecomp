[CmdletBinding()]
param(
    [Parameter(Position = 0)]
    [string]$RomPath = $env:SUPER_TENNIS_ROM,
    [string]$Python,
    [switch]$SkipGenerate
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version 2.0

$Root = Split-Path -Parent (Split-Path -Parent $MyInvocation.MyCommand.Path)
$ExpectedSha256 = '6e45a80ea148654514cb4e8604a0ffcbc726946e70f9e0b9860e36c0f3fa4877'

if (-not $SkipGenerate -and [string]::IsNullOrWhiteSpace($RomPath)) {
    throw 'Usage: tools\regenerate.ps1 -RomPath "C:\path\to\Super Tennis (USA).sfc"'
}
if (-not $SkipGenerate -and -not (Test-Path -LiteralPath $RomPath -PathType Leaf)) {
    throw "ROM not found: $RomPath"
}

if (-not $SkipGenerate) {
    $RomPath = (Resolve-Path -LiteralPath $RomPath).Path
}
$Config = Join-Path $Root 'config\bank00.cfg'
$Emitter = Join-Path $Root 'snesrecomp\tools\v2_emit.py'
if (-not (Test-Path $Config -PathType Leaf)) {
    throw 'Missing config\bank00.cfg. Restore the tracked file before regenerating.'
}
if (-not (Test-Path $Emitter -PathType Leaf)) {
    throw 'Missing patched snesrecomp dependency. Initialize the submodules first.'
}

if (-not $SkipGenerate) {
    $actualSha256 = (Get-FileHash -LiteralPath $RomPath -Algorithm SHA256).Hash.ToLowerInvariant()
    if ($actualSha256 -ne $ExpectedSha256) {
        throw "Unsupported ROM SHA-256: $actualSha256`nExpected: $ExpectedSha256"
    }
}

$candidates = @()
if (-not [string]::IsNullOrWhiteSpace($Python)) {
    if (Test-Path -LiteralPath $Python -PathType Leaf) {
        $candidates += [pscustomobject]@{
            Executable = (Resolve-Path -LiteralPath $Python).Path
            Prefix = @()
        }
    } else {
        $resolvedPython = Get-Command $Python -ErrorAction SilentlyContinue
        if ($resolvedPython) {
            $candidates += [pscustomobject]@{
                Executable = $resolvedPython.Source
                Prefix = @()
            }
        }
    }
} else {
    $resolvedPython = Get-Command python.exe -ErrorAction SilentlyContinue
    if (-not $resolvedPython) {
        $resolvedPython = Get-Command python -ErrorAction SilentlyContinue
    }
    if ($resolvedPython) {
        $candidates += [pscustomobject]@{
            Executable = $resolvedPython.Source
            Prefix = @()
        }
    }
    $pyLauncher = Get-Command py.exe -ErrorAction SilentlyContinue
    if (-not $pyLauncher) {
        $pyLauncher = Get-Command py -ErrorAction SilentlyContinue
    }
    if ($pyLauncher) {
        $candidates += [pscustomobject]@{
            Executable = $pyLauncher.Source
            Prefix = @('-3')
        }
    }
}

$PythonExecutable = $null
$PythonPrefix = @()
$versionText = $null
foreach ($candidate in $candidates) {
    $candidateExecutable = [string]$candidate.Executable
    $candidatePrefix = @($candidate.Prefix)
    $candidateVersion = $null
    $candidateExitCode = -1
    $savedErrorPreference = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $candidateVersion = & $candidateExecutable @candidatePrefix --version 2>&1
        $candidateExitCode = $LASTEXITCODE
    } catch {
        $candidateExitCode = -1
    } finally {
        $ErrorActionPreference = $savedErrorPreference
    }

    $candidateVersion = ($candidateVersion | Out-String).Trim()
    if ($candidateExitCode -eq 0 -and
        $candidateVersion -match '^Python (\d+)\.(\d+)(?:\.(\d+))?') {
        $major = [int]$matches[1]
        $minor = [int]$matches[2]
        if ($major -gt 3 -or ($major -eq 3 -and $minor -ge 11)) {
            $PythonExecutable = $candidateExecutable
            $PythonPrefix = $candidatePrefix
            $versionText = $candidateVersion.Substring(7)
            break
        }
    }
}
if (-not $PythonExecutable) {
    throw 'Python 3.11 or newer is required. Install Python and retry.'
}

Write-Host "Using Python $versionText"

& $PythonExecutable @PythonPrefix (Join-Path $Root 'tools\apply-snesrecomp-patches.py')
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
if ($SkipGenerate) {
    return
}
& $PythonExecutable @PythonPrefix (Join-Path $Root 'tools\generate-normal.py') $RomPath
if ($LASTEXITCODE -ne 0) {
    exit $LASTEXITCODE
}
