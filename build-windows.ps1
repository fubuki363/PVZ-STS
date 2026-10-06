param(
    [string]$VcpkgRoot = (Join-Path $PSScriptRoot '.tools\vcpkg'),
    [string]$BuildDirectory = (Join-Path $PSScriptRoot 'build-windows'),
    [int]$Jobs = 8,
    [switch]$DisplayTests,
    [switch]$GameplayTests,
    [string]$ExecutableName = 'pvz-portable-glass.exe'
)
$ErrorActionPreference = 'Stop'
$toolchain = Join-Path $VcpkgRoot 'scripts\buildsystems\vcpkg.cmake'
if (-not (Test-Path -LiteralPath $toolchain)) {
    throw "vcpkg was not found at $VcpkgRoot"
}
# Static libraries and CRT make the resulting EXE independent of installed DLLs.
$pvzDisplayTests = if ($DisplayTests) { 'ON' } else { 'OFF' }
$pvzGameplayTests = if ($GameplayTests) { 'ON' } else { 'OFF' }
& cmake -S $PSScriptRoot -B $BuildDirectory -G 'Visual Studio 17 2022' -A x64 `
    "-DCMAKE_TOOLCHAIN_FILE=$toolchain" '-DVCPKG_TARGET_TRIPLET=x64-windows-static' `
    '-DVCPKG_MANIFEST_MODE=OFF' '-DPVZ_DEBUG=OFF' '-DLIMBO_PAGE=OFF' `
    '-DDO_FIX_BUGS=OFF' '-DCONSOLE=OFF' "-DPVZ_DISPLAY_TESTS=$pvzDisplayTests" "-DPVZ_GAMEPLAY_TESTS=$pvzGameplayTests"
if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed' }
& cmake --build $BuildDirectory --config Release --parallel $Jobs
if ($LASTEXITCODE -ne 0) { throw 'Windows compilation failed' }
$outputDirectory = Join-Path $PSScriptRoot 'dist'
New-Item -ItemType Directory -Force -Path $outputDirectory | Out-Null
Copy-Item -LiteralPath (Join-Path $BuildDirectory 'Release\pvz-portable.exe') -Destination (Join-Path $outputDirectory $ExecutableName) -Force
Copy-Item -LiteralPath (Join-Path $PSScriptRoot 'COPYING'),(Join-Path $PSScriptRoot 'LICENSE') -Destination $outputDirectory -Force
$licensesDirectory = Join-Path $outputDirectory 'licenses'
New-Item -ItemType Directory -Force -Path $licensesDirectory | Out-Null
foreach ($dependency in 'sdl2','libjpeg-turbo','libpng','libogg','libvorbis','libopenmpt','mpg123','zlib') {
    $notice = Join-Path $VcpkgRoot "installed\x64-windows-static\share\$dependency\copyright"
    if (Test-Path -LiteralPath $notice) {
        Copy-Item -LiteralPath $notice -Destination (Join-Path $licensesDirectory "$dependency.txt") -Force
    }
}
Write-Host "Built $outputDirectory\$ExecutableName"
