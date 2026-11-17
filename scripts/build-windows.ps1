$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$expected = '50718bceaef9953a6899abddb34b4ecbb36817b5ef3967e5a96d3b0c6149b9f0'
if ((Get-FileHash assets/imagem.png -Algorithm SHA256).Hash.ToLower() -ne $expected) { throw 'Immutable image hash mismatch' }
New-Item -ItemType Directory -Force build-win | Out-Null
function Invoke-Checked([string]$Stage, [scriptblock]$Command) {
    & $Command 2>&1 | Tee-Object -FilePath "build-win/$Stage.log"
    $result = $LASTEXITCODE
    if ($result) {
        Get-Content "build-win/$Stage.log" | Select-String -Pattern 'error|failed|Exception|what\(\)|mapping|Subprocess|control|restore' | Select-Object -Last 25 | ForEach-Object {
            $message = $_.ToString().Replace('%', '%25').Replace("`r", '%0D').Replace("`n", '%0A')
            Write-Host "::error::$Stage - $message"
        }
        exit $result
    }
}
Invoke-Checked 'configure' { cmake -S . -B build-win -A x64 }
Invoke-Checked 'compile' { cmake --build build-win --config Release --parallel 2 }
Invoke-Checked 'tests' { ctest --test-dir build-win -C Release --output-on-failure }
