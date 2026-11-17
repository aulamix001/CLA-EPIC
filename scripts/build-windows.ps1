$ErrorActionPreference = 'Stop'
Set-Location (Split-Path $PSScriptRoot -Parent)
$expected = '50718bceaef9953a6899abddb34b4ecbb36817b5ef3967e5a96d3b0c6149b9f0'
if ((Get-FileHash assets/imagem.png -Algorithm SHA256).Hash.ToLower() -ne $expected) { throw 'Immutable image hash mismatch' }
cmake -S . -B build-win -A x64
if ($LASTEXITCODE) { exit $LASTEXITCODE }
cmake --build build-win --config Release --parallel 2
if ($LASTEXITCODE) { exit $LASTEXITCODE }
ctest --test-dir build-win -C Release --output-on-failure
if ($LASTEXITCODE) { exit $LASTEXITCODE }
