param([switch]$Test)
$ErrorActionPreference = 'Stop'
$Root = $PSScriptRoot
Push-Location $Root
try {
    $Gcc = Get-Command g++.exe -ErrorAction SilentlyContinue
    if ($Gcc) { $Compiler = $Gcc.Source }
    elseif (Test-Path 'C:\msys64\ucrt64\bin\g++.exe') { $Compiler = 'C:\msys64\ucrt64\bin\g++.exe' }
    else { throw 'g++ nao encontrado. Instale o GCC no MSYS2 UCRT64. Veja o README.md.' }
    $env:Path = (Split-Path $Compiler) + ';' + $env:Path
    $Flags = @('-std=c++17', '-O2', '-Wall', '-Wextra', '-Wpedantic')
    Write-Host 'Compilando Amber Grove...' -ForegroundColor Cyan
    & $Compiler @Flags '-DUNICODE' '-D_UNICODE' 'src/main.cpp' 'src/game.cpp' 'src/render.cpp' 'src/platform_windows.cpp' '-mwindows' '-static' '-static-libgcc' '-static-libstdc++' '-lgdi32' '-luser32' '-lwinmm' '-o' 'AmberGrove.exe'
    if ($LASTEXITCODE -ne 0) { throw 'Falha na compilacao. Confira as mensagens acima.' }
    if ($Test) {
        New-Item -ItemType Directory -Force 'build' | Out-Null
        & $Compiler @Flags '-Isrc' 'tests/tests.cpp' 'src/game.cpp' 'src/render.cpp' '-static' '-o' 'build/amber_tests.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Falha na compilacao dos testes.' }
        & '.\build\amber_tests.exe'
        if ($LASTEXITCODE -ne 0) { throw 'Os testes falharam.' }
    }
    Write-Host 'Pronto! Abra AmberGrove.exe para jogar.' -ForegroundColor Green
} finally { Pop-Location }
