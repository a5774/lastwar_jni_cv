param(
    [switch]$Clean
)

$ErrorActionPreference = 'Stop'
Set-StrictMode -Version Latest

$ProjectCpp = 'E:\lastwar_jni_cv\app\src\main\cpp'
$ThirdParty = Join-Path $ProjectCpp 'third_party'
$SdkRoot = 'E:\android-sdk'
$NdkRoot = Join-Path $SdkRoot 'ndk\27.0.12077973'
$CMakeRoot = Join-Path $SdkRoot 'cmake\3.22.1\bin'
$CMakeExe = Join-Path $CMakeRoot 'cmake.exe'
$NinjaExe = Join-Path $CMakeRoot 'ninja.exe'
$Toolchain = Join-Path $NdkRoot 'build\cmake\android.toolchain.cmake'

$BuildRoot = Join-Path $SdkRoot 'ocr-build\arm64-v8a'
$Prefix = Join-Path $BuildRoot 'prefix'
$CpuSource = Join-Path $ThirdParty 'cpu_features-0.9.0'
$LeptonicaSource = Join-Path $ThirdParty 'leptonica-1.85.0'
$TesseractSource = Join-Path $ThirdParty 'tesseract-5.5.4'
$CpuBuild = Join-Path $BuildRoot 'cpu_features'
$LeptonicaBuild = Join-Path $BuildRoot 'leptonica'
$TesseractBuild = Join-Path $BuildRoot 'tesseract'
$OutputDir = Join-Path $ProjectCpp 'libs\arm64-v8a'
$LeptonicaPackage = Join-Path $Prefix 'lib\cmake\leptonica'

$RequiredPaths = @(
    $CMakeExe,
    $NinjaExe,
    $Toolchain,
    (Join-Path $CpuSource 'CMakeLists.txt'),
    (Join-Path $LeptonicaSource 'CMakeLists.txt'),
    (Join-Path $TesseractSource 'CMakeLists.txt')
)
foreach ($Path in $RequiredPaths) {
    if (-not (Test-Path -LiteralPath $Path)) {
        throw "Required build input not found: $Path"
    }
}

New-Item -ItemType Directory -Force -Path $BuildRoot, $Prefix, $OutputDir | Out-Null

function Invoke-CMake {
    param(
        [Parameter(Mandatory = $true)]
        [string[]]$Arguments
    )

    & $CMakeExe @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "CMake failed with exit code ${LASTEXITCODE}: $($Arguments -join ' ')"
    }
}

$CommonConfigureArgs = @(
    '-G', 'Ninja',
    "-DCMAKE_MAKE_PROGRAM=$NinjaExe",
    "-DCMAKE_TOOLCHAIN_FILE=$Toolchain",
    '-DANDROID_ABI=arm64-v8a',
    '-DANDROID_PLATFORM=android-28',
    '-DANDROID_STL=c++_shared',
    '-DCMAKE_BUILD_TYPE=Release',
    "-DCMAKE_INSTALL_PREFIX=$Prefix"
)
$CleanBuildArgs = @()
if ($Clean) {
    $CleanBuildArgs = @('--clean-first')
}

Write-Host 'Configuring cpu_features...'
$CpuConfigureArgs = @('-S', $CpuSource, '-B', $CpuBuild) +
    $CommonConfigureArgs + @(
        '-DBUILD_SHARED_LIBS=OFF',
        '-DBUILD_EXECUTABLE=OFF',
        '-DBUILD_TESTING=OFF',
        '-DENABLE_INSTALL=ON'
    )
Invoke-CMake -Arguments $CpuConfigureArgs
Invoke-CMake -Arguments (@('--build', $CpuBuild) + $CleanBuildArgs + @('--parallel', '4'))
Invoke-CMake -Arguments @('--install', $CpuBuild)

Write-Host 'Configuring Leptonica...'
$LeptonicaConfigureArgs = @('-S', $LeptonicaSource, '-B', $LeptonicaBuild) +
    $CommonConfigureArgs + @(
        '-DBUILD_SHARED_LIBS=ON',
        '-DBUILD_PROG=OFF',
        '-DSW_BUILD=OFF',
        '-DENABLE_ZLIB=OFF',
        '-DENABLE_PNG=OFF',
        '-DENABLE_GIF=OFF',
        '-DENABLE_JPEG=OFF',
        '-DENABLE_TIFF=OFF',
        '-DENABLE_WEBP=OFF',
        '-DENABLE_OPENJPEG=OFF'
    )
Invoke-CMake -Arguments $LeptonicaConfigureArgs
Invoke-CMake -Arguments (@('--build', $LeptonicaBuild) + $CleanBuildArgs +
    @('--target', 'leptonica', '--parallel', '4'))
Invoke-CMake -Arguments @('--install', $LeptonicaBuild)

Write-Host 'Configuring Tesseract...'
$TesseractConfigureArgs = @('-S', $TesseractSource, '-B', $TesseractBuild) +
    $CommonConfigureArgs + @(
        "-DCMAKE_PREFIX_PATH=$Prefix",
        "-DLeptonica_DIR=$LeptonicaPackage",
        '-DBUILD_SHARED_LIBS=ON',
        '-DBUILD_TESTS=OFF',
        '-DBUILD_TRAINING_TOOLS=OFF',
        '-DDISABLED_LEGACY_ENGINE=ON',
        '-DDISABLE_ARCHIVE=ON',
        '-DDISABLE_CURL=ON',
        '-DDISABLE_TIFF=ON',
        '-DGRAPHICS_DISABLED=ON',
        '-DOPENMP_BUILD=OFF',
        '-DUSE_SYSTEM_ICU=OFF',
        '-DENABLE_PRECOMPILED_HEADERS=OFF',
        '-DENABLE_CCACHE=OFF'
    )
Invoke-CMake -Arguments $TesseractConfigureArgs
Invoke-CMake -Arguments (@('--build', $TesseractBuild) + $CleanBuildArgs +
    @('--target', 'libtesseract', '--parallel', '4'))

$LeptonicaOutput = Join-Path $Prefix 'lib\libleptonica.so'
$TesseractOutput = Join-Path $TesseractBuild 'libtesseract.so'
foreach ($Output in @($LeptonicaOutput, $TesseractOutput)) {
    if (-not (Test-Path -LiteralPath $Output)) {
        throw "Expected shared library was not produced: $Output"
    }
}

Copy-Item -LiteralPath $LeptonicaOutput -Destination $OutputDir -Force
# The Tesseract install target also installs the optional CLI, which is not built.
Copy-Item -LiteralPath $TesseractOutput -Destination $OutputDir -Force

Write-Host 'Build complete. Android shared libraries:'
Get-Item -LiteralPath (Join-Path $OutputDir 'libleptonica.so'),
    (Join-Path $OutputDir 'libtesseract.so') |
    Select-Object Name, Length, FullName
