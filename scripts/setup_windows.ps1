$ErrorActionPreference = 'Stop'
$projectRoot = Split-Path -Parent $PSScriptRoot
Set-Location -LiteralPath $projectRoot
New-Item -ItemType Directory -Force tools | Out-Null
if (-not (Test-Path -LiteralPath 'tools/llvm-mingw-20261020-ucrt-x86_64/bin/clang++.exe')) {
    $url = 'https://github.com/mstorsjo/llvm-mingw/releases/download/20261020/llvm-mingw-20261020-ucrt-x86_64.zip'
    curl.exe -fL --retry 2 -o tools/llvm-mingw.zip $url
    if ($LASTEXITCODE -ne 0) { throw 'Falha baixando LLVM-MinGW.' }
    python -m zipfile -e tools/llvm-mingw.zip tools
    if ($LASTEXITCODE -ne 0) { throw 'Falha extraindo LLVM-MinGW.' }
    Remove-Item -LiteralPath 'tools/llvm-mingw.zip'
}
if (-not (Test-Path -LiteralPath 'tools/python/cmake/data/bin/cmake.exe')) {
    python -m pip install --target tools/python cmake==4.4.4 ninja==1.13.2
    if ($LASTEXITCODE -ne 0) { throw 'Falha instalando CMake e Ninja locais.' }
}
if (-not (Test-Path -LiteralPath 'tools/SDL2-2.32.10/x86_64-w64-mingw32/bin/SDL2.dll')) {
    curl.exe -fL --retry 2 -o tools/SDL2.zip https://github.com/libsdl-org/SDL/releases/download/release-2.32.10/SDL2-devel-2.32.10-mingw.zip
    if ($LASTEXITCODE -ne 0) { throw 'Falha baixando SDL2.' }
    python -m zipfile -e tools/SDL2.zip tools
    if ($LASTEXITCODE -ne 0) { throw 'Falha extraindo SDL2.' }
    Remove-Item -LiteralPath 'tools/SDL2.zip'
}
Write-Output 'Dependencias locais prontas. Use python scripts/build_gow2.py --iso "C:\caminho\God of War II (USA).iso" --package'
