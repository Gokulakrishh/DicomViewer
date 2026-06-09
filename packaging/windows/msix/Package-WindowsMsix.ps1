param(
    [string]$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot "..\..\..")).Path,
    [string]$BuildDir,
    [string]$OutputRoot,
    [ValidateSet("Debug", "Release", "RelWithDebInfo", "MinSizeRel")]
    [string]$Config = "Release",
    [string]$Generator = "Ninja",
    [string]$Architecture = "x64",
    [string]$QtRoot,
    [string]$QtBin,
    [string]$VTK_DIR,
    [string]$VtkBin,
    [string]$GDCM_DIR,
    [string]$GdcmBin,
    [string]$MsvcRuntimeDir,
    [string[]]$ExtraBinDirs = @(),
    [string]$PublisherName = "CN=Cross Axial Dicom Viewer Development",
    [string]$PublisherDisplayName = "Cross Axial Dicom Viewer",
    [string]$PackageIdentityName = "CrossAxialDicomViewer",
    [string]$CertificatePath,
    [securestring]$CertificatePassword,
    [switch]$CreateTestCertificate,
    [switch]$SkipConfigure,
    [switch]$SkipBuild,
    [switch]$SkipSign,
    [switch]$KeepStage
)

$ErrorActionPreference = "Stop"

if (-not $BuildDir) {
    $BuildDir = Join-Path $RepoRoot "build-windows-msix"
}
if (-not $OutputRoot) {
    $OutputRoot = Join-Path $RepoRoot "artifacts\windows-msix"
}

function Write-Step([string]$Message) {
    Write-Host ""
    Write-Host "==> $Message" -ForegroundColor Cyan
}

function Invoke-NativeCommand([string]$FilePath, [string[]]$ArgumentList) {
    Write-Host "+ $FilePath $($ArgumentList -join ' ')" -ForegroundColor DarkGray
    & $FilePath @ArgumentList
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE."
    }
}

function Get-CMakeString([string]$Name, [string]$Fallback) {
    $cmakePath = Join-Path $RepoRoot "CMakeLists.txt"
    $content = Get-Content -Raw -Path $cmakePath
    $escapedName = [regex]::Escape($Name)
    $pattern = 'set\(\s*' + $escapedName + '\s+"([^"]+)"'
    $match = [regex]::Match($content, $pattern)
    if ($match.Success) {
        return $match.Groups[1].Value
    }
    return $Fallback
}

function Find-Tool([string]$ToolName, [string[]]$Roots) {
    $command = Get-Command $ToolName -ErrorAction SilentlyContinue
    if ($command) {
        return $command.Source
    }

    foreach ($root in $Roots) {
        if (-not $root -or -not (Test-Path $root)) {
            continue
        }
        $match = Get-ChildItem -Path $root -Filter $ToolName -Recurse -ErrorAction SilentlyContinue |
            Sort-Object FullName -Descending |
            Select-Object -First 1
        if ($match) {
            return $match.FullName
        }
    }

    throw "Unable to find $ToolName. Install the Windows SDK or add the tool to PATH."
}

function Find-BuiltExecutable([string]$Root, [string]$BuildConfig, [string]$ProductName) {
    $candidates = @(
        (Join-Path $Root "$BuildConfig\$ProductName.exe"),
        (Join-Path $Root "$ProductName.exe"),
        (Join-Path $Root "bin\$BuildConfig\$ProductName.exe"),
        (Join-Path $Root "bin\$ProductName.exe")
    )

    foreach ($candidate in $candidates) {
        if (Test-Path $candidate) {
            return (Resolve-Path $candidate).Path
        }
    }

    $found = Get-ChildItem -Path $Root -Filter "$ProductName.exe" -Recurse -ErrorAction SilentlyContinue |
        Sort-Object LastWriteTime -Descending |
        Select-Object -First 1
    if ($found) {
        return $found.FullName
    }

    throw "Unable to find $ProductName.exe under $Root."
}

function ConvertTo-PlainText([securestring]$Value) {
    if (-not $Value) {
        return $null
    }
    $bstr = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($Value)
    try {
        return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($bstr)
    }
    finally {
        [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($bstr)
    }
}

function ConvertTo-XmlText([string]$Value) {
    return [Security.SecurityElement]::Escape($Value)
}

function Copy-DllsFromDirectory([string]$SourceDir, [string]$DestinationDir) {
    if (-not $SourceDir) {
        return
    }
    if (-not (Test-Path $SourceDir)) {
        Write-Warning "DLL directory not found: $SourceDir"
        return
    }

    Get-ChildItem -Path $SourceDir -Filter "*.dll" -File -ErrorAction SilentlyContinue |
        ForEach-Object {
            Copy-Item -Path $_.FullName -Destination $DestinationDir -Force
        }
}

function Get-MsvcRuntimeSearchRoots([string]$ExplicitRuntimeDir) {
    $roots = @()
    if ($ExplicitRuntimeDir) {
        $roots += $ExplicitRuntimeDir
    }

    $programFiles = ${env:ProgramFiles}
    if ($programFiles) {
        foreach ($edition in @("BuildTools", "Community", "Professional", "Enterprise")) {
            $redistRoot = Join-Path $programFiles "Microsoft Visual Studio\2022\$edition\VC\Redist\MSVC"
            if (Test-Path $redistRoot) {
                $roots += $redistRoot
            }
        }
    }

    $systemRoot = $env:SystemRoot
    if ($systemRoot) {
        $roots += (Join-Path $systemRoot "System32")
    }

    return $roots
}

function Copy-MsvcRuntimeDlls([string]$DestinationDir, [string]$ExplicitRuntimeDir) {
    $runtimeDlls = @(
        "msvcp140.dll",
        "msvcp140_1.dll",
        "msvcp140_2.dll",
        "vcruntime140.dll",
        "vcruntime140_1.dll",
        "concrt140.dll"
    )

    $searchRoots = Get-MsvcRuntimeSearchRoots -ExplicitRuntimeDir $ExplicitRuntimeDir

    foreach ($dllName in $runtimeDlls) {
        $source = $null
        foreach ($root in $searchRoots) {
            if (-not $root -or -not (Test-Path $root)) {
                continue
            }

            $directCandidate = Join-Path $root $dllName
            if (Test-Path $directCandidate) {
                $source = $directCandidate
                break
            }

            $found = Get-ChildItem -Path $root -Filter $dllName -File -Recurse -ErrorAction SilentlyContinue |
                Where-Object { $_.FullName -match "\\x64\\" -or $_.DirectoryName -match "\\System32$" } |
                Sort-Object FullName -Descending |
                Select-Object -First 1
            if ($found) {
                $source = $found.FullName
                break
            }
        }

        if (-not $source) {
            if ($dllName -like "msvcp140_*") {
                Write-Warning "Optional MSVC runtime DLL '$dllName' was not found. If the packaged app reports it missing, pass -MsvcRuntimeDir to a newer Microsoft.VC143.CRT folder."
                continue
            }

            throw "Unable to find MSVC runtime DLL '$dllName'. Install the Microsoft Visual C++ 2015-2022 x64 Redistributable or pass -MsvcRuntimeDir to the folder containing the runtime DLLs."
        }

        Copy-Item -Path $source -Destination $DestinationDir -Force
        Write-Host "Copied $dllName from $source"
    }
}

function New-PngAsset(
    [string]$SourcePath,
    [string]$DestinationPath,
    [int]$Width,
    [int]$Height,
    [string]$Background = "Transparent",
    [double]$FillRatio = 0.72
) {
    Add-Type -AssemblyName System.Drawing

    $source = [System.Drawing.Image]::FromFile($SourcePath)
    try {
        $bitmap = New-Object System.Drawing.Bitmap $Width, $Height, ([System.Drawing.Imaging.PixelFormat]::Format32bppArgb)
        try {
            $graphics = [System.Drawing.Graphics]::FromImage($bitmap)
            try {
                $graphics.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::HighQuality
                $graphics.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::HighQualityBicubic
                $graphics.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::HighQuality
                $graphics.CompositingQuality = [System.Drawing.Drawing2D.CompositingQuality]::HighQuality

                if ($Background -eq "Transparent") {
                    $graphics.Clear([System.Drawing.Color]::Transparent)
                }
                else {
                    $graphics.Clear([System.Drawing.ColorTranslator]::FromHtml($Background))
                }

                $scale = [Math]::Min($Width / $source.Width, $Height / $source.Height) * $FillRatio
                $targetWidth = [int][Math]::Round($source.Width * $scale)
                $targetHeight = [int][Math]::Round($source.Height * $scale)
                $x = [int][Math]::Round(($Width - $targetWidth) / 2)
                $y = [int][Math]::Round(($Height - $targetHeight) / 2)

                $graphics.DrawImage($source, $x, $y, $targetWidth, $targetHeight)
            }
            finally {
                $graphics.Dispose()
            }

            $directory = Split-Path -Parent $DestinationPath
            if (-not (Test-Path $directory)) {
                New-Item -ItemType Directory -Path $directory | Out-Null
            }
            $bitmap.Save($DestinationPath, [System.Drawing.Imaging.ImageFormat]::Png)
        }
        finally {
            $bitmap.Dispose()
        }
    }
    finally {
        $source.Dispose()
    }
}

$productName = Get-CMakeString "DICOMVIEWER_PRODUCT_NAME" "CrossAxialDicomViewer"
$displayName = Get-CMakeString "DICOMVIEWER_DISPLAY_NAME" "Cross Axial Dicom Viewer"
$versionMajor = Get-CMakeString "DICOMVIEWER_VERSION_MAJOR" "1"
$versionMinor = Get-CMakeString "DICOMVIEWER_VERSION_MINOR" "0"
$versionRelease = Get-CMakeString "DICOMVIEWER_VERSION_RELEASE" "0"
$versionBuild = Get-CMakeString "DICOMVIEWER_VERSION_BUILD" "1"
$version = "$versionMajor.$versionMinor.$versionRelease.$versionBuild"
$description = "$displayName is a local-first DICOM viewer under controlled non-diagnostic development."

if (-not $QtBin -and $QtRoot) {
    $QtBin = Join-Path $QtRoot "bin"
}

if ($QtBin -and -not (Test-Path (Join-Path $QtBin "windeployqt.exe"))) {
    throw "windeployqt.exe was not found in QtBin: $QtBin"
}

$stageRoot = Join-Path $OutputRoot "stage"
$packageRoot = Join-Path $stageRoot "package-root"
$assetsRoot = Join-Path $packageRoot "Assets"
$certRoot = Join-Path $OutputRoot "certs"
$manifestTemplate = Join-Path $PSScriptRoot "Package.appxmanifest.in"
$noticeSource = Join-Path $PSScriptRoot "NOTICE-NON-DIAGNOSTIC.txt"
$sourceIcon = Join-Path $RepoRoot "src\resources\icons\icon_512.png"
if (-not (Test-Path $sourceIcon)) {
    $sourceIcon = Join-Path $RepoRoot "src\resources\icons\icon_256.png"
}

New-Item -ItemType Directory -Path $OutputRoot -Force | Out-Null

if (-not $SkipConfigure) {
    Write-Step "Configuring CMake"
    $configureArgs = @("-S", $RepoRoot, "-B", $BuildDir, "-G", $Generator, "-DDICOMVIEWER_ENABLE_VTK=ON")
    if ($Generator -match "Visual Studio") {
        $configureArgs += "-A"
        $configureArgs += $Architecture
    }
    else {
        $configureArgs += "-DCMAKE_BUILD_TYPE=$Config"
    }
    if ($QtRoot) {
        $configureArgs += "-DCMAKE_PREFIX_PATH=$QtRoot"
    }
    if ($VTK_DIR) {
        $configureArgs += "-DVTK_DIR=$VTK_DIR"
    }
    if ($GDCM_DIR) {
        $configureArgs += "-DGDCM_DIR=$GDCM_DIR"
    }
    Invoke-NativeCommand -FilePath "cmake" -ArgumentList $configureArgs
}

if (-not $SkipBuild) {
    Write-Step "Building $productName $Config"
    $buildArgs = @("--build", $BuildDir, "--config", $Config, "--target", $productName, "--parallel", "4")
    Invoke-NativeCommand -FilePath "cmake" -ArgumentList $buildArgs
}

Write-Step "Staging application payload"
if (Test-Path $packageRoot) {
    Remove-Item -Path $packageRoot -Recurse -Force
}
New-Item -ItemType Directory -Path $packageRoot -Force | Out-Null
New-Item -ItemType Directory -Path $assetsRoot -Force | Out-Null

$builtExe = Find-BuiltExecutable -Root $BuildDir -BuildConfig $Config -ProductName $productName
$stageExe = Join-Path $packageRoot "$productName.exe"
Copy-Item -Path $builtExe -Destination $stageExe -Force

if (-not $QtBin) {
    $windeployqt = Find-Tool "windeployqt.exe" @($env:PATH)
}
else {
    $windeployqt = Join-Path $QtBin "windeployqt.exe"
}

Write-Step "Deploying Qt runtime"
Invoke-NativeCommand -FilePath $windeployqt -ArgumentList @("--release", "--compiler-runtime", "--no-translations", $stageExe)

Write-Step "Copying MSVC runtime DLLs"
Copy-MsvcRuntimeDlls -DestinationDir $packageRoot -ExplicitRuntimeDir $MsvcRuntimeDir

Write-Step "Copying VTK/GDCM/extra runtime DLLs"
Copy-DllsFromDirectory -SourceDir $VtkBin -DestinationDir $packageRoot
Copy-DllsFromDirectory -SourceDir $GdcmBin -DestinationDir $packageRoot
foreach ($extraDir in $ExtraBinDirs) {
    Copy-DllsFromDirectory -SourceDir $extraDir -DestinationDir $packageRoot
}

Get-ChildItem -Path $packageRoot -Include "*.pdb", "*.ilk", "*.exp", "*.lib" -File -Recurse -ErrorAction SilentlyContinue |
    Remove-Item -Force

Copy-Item -Path $noticeSource -Destination (Join-Path $packageRoot "NOTICE-NON-DIAGNOSTIC.txt") -Force

Write-Step "Generating MSIX visual assets"
New-PngAsset -SourcePath $sourceIcon -DestinationPath (Join-Path $assetsRoot "Square44x44Logo.png") -Width 44 -Height 44
New-PngAsset -SourcePath $sourceIcon -DestinationPath (Join-Path $assetsRoot "Square71x71Logo.png") -Width 71 -Height 71
New-PngAsset -SourcePath $sourceIcon -DestinationPath (Join-Path $assetsRoot "Square150x150Logo.png") -Width 150 -Height 150
New-PngAsset -SourcePath $sourceIcon -DestinationPath (Join-Path $assetsRoot "Square310x310Logo.png") -Width 310 -Height 310
New-PngAsset -SourcePath $sourceIcon -DestinationPath (Join-Path $assetsRoot "Wide310x150Logo.png") -Width 310 -Height 150 -Background "#17365D" -FillRatio 0.55
New-PngAsset -SourcePath $sourceIcon -DestinationPath (Join-Path $assetsRoot "SplashScreen.png") -Width 620 -Height 300 -Background "#17365D" -FillRatio 0.45
New-PngAsset -SourcePath $sourceIcon -DestinationPath (Join-Path $assetsRoot "StoreLogo.png") -Width 50 -Height 50

Write-Step "Writing AppxManifest.xml"
$manifest = Get-Content -Raw -Path $manifestTemplate
$manifest = $manifest.Replace("__IDENTITY_NAME__", (ConvertTo-XmlText $PackageIdentityName))
$manifest = $manifest.Replace("__PUBLISHER__", (ConvertTo-XmlText $PublisherName))
$manifest = $manifest.Replace("__VERSION__", (ConvertTo-XmlText $version))
$manifest = $manifest.Replace("__DISPLAY_NAME__", (ConvertTo-XmlText $displayName))
$manifest = $manifest.Replace("__PUBLISHER_DISPLAY_NAME__", (ConvertTo-XmlText $PublisherDisplayName))
$manifest = $manifest.Replace("__DESCRIPTION__", (ConvertTo-XmlText $description))
$manifest = $manifest.Replace("__EXECUTABLE__", "$productName.exe")
Set-Content -Path (Join-Path $packageRoot "AppxManifest.xml") -Value $manifest -Encoding UTF8

$programFilesX86 = ${env:ProgramFiles(x86)}
$windowsKitsRoot = $null
if ($programFilesX86) {
    $windowsKitsRoot = Join-Path $programFilesX86 "Windows Kits\10\bin"
}
$makeAppx = Find-Tool "makeappx.exe" @($windowsKitsRoot)
$signTool = $null
if (-not $SkipSign) {
    $signTool = Find-Tool "signtool.exe" @($windowsKitsRoot)
}

$msixPath = Join-Path $OutputRoot "$productName-$version-x64.msix"
if (Test-Path $msixPath) {
    Remove-Item -Path $msixPath -Force
}

Write-Step "Creating MSIX package"
Invoke-NativeCommand -FilePath $makeAppx -ArgumentList @("pack", "/d", $packageRoot, "/p", $msixPath, "/overwrite")

$cerPath = $null
if ($CreateTestCertificate) {
    Write-Step "Creating local test signing certificate"
    if (-not $CertificatePassword) {
        throw "Use -CertificatePassword (Read-Host -AsSecureString) when creating a test certificate."
    }
    New-Item -ItemType Directory -Path $certRoot -Force | Out-Null
    if (-not $CertificatePath) {
        $CertificatePath = Join-Path $certRoot "$productName-TestCertificate.pfx"
    }
    $cerPath = [System.IO.Path]::ChangeExtension($CertificatePath, ".cer")
    $cert = New-SelfSignedCertificate `
        -Type Custom `
        -Subject $PublisherName `
        -KeyUsage DigitalSignature `
        -FriendlyName "$displayName MSIX Test Certificate" `
        -CertStoreLocation "Cert:\CurrentUser\My" `
        -TextExtension @("2.5.29.37={text}1.3.6.1.5.5.7.3.3")
    Export-PfxCertificate -Cert $cert -FilePath $CertificatePath -Password $CertificatePassword | Out-Null
    Export-Certificate -Cert $cert -FilePath $cerPath | Out-Null
}

if (-not $SkipSign) {
    if (-not $CertificatePath) {
        Write-Warning "No certificate path supplied. MSIX was created but not signed. Use -CreateTestCertificate or -CertificatePath for local install testing."
    }
    else {
        Write-Step "Signing MSIX package"
        if (-not $CertificatePassword) {
            throw "Use -CertificatePassword (Read-Host -AsSecureString) when signing with a PFX certificate."
        }
        $plainPassword = ConvertTo-PlainText $CertificatePassword
        try {
            Invoke-NativeCommand -FilePath $signTool -ArgumentList @("sign", "/fd", "SHA256", "/f", $CertificatePath, "/p", $plainPassword, $msixPath)
        }
        finally {
            $plainPassword = $null
        }
    }
}

$gitCommit = "unknown"
try {
    $gitCommit = (& git -C $RepoRoot rev-parse HEAD).Trim()
}
catch {
    $gitCommit = "unknown"
}

$hash = Get-FileHash -Algorithm SHA256 -Path $msixPath
$artifactRecord = [ordered]@{
    product = $productName
    displayName = $displayName
    version = $version
    packageIdentityName = $PackageIdentityName
    publisher = $PublisherName
    configuration = $Config
    gitCommit = $gitCommit
    createdUtc = (Get-Date).ToUniversalTime().ToString("o")
    packagePath = $msixPath
    sha256 = $hash.Hash
    certificatePath = $CertificatePath
    certificatePublicPath = $cerPath
    nonDiagnosticNotice = "NOTICE-NON-DIAGNOSTIC.txt"
}

$artifactRecordPath = Join-Path $OutputRoot "$productName-$version-x64.artifact.json"
$artifactRecord | ConvertTo-Json -Depth 4 | Set-Content -Path $artifactRecordPath -Encoding UTF8

if (-not $KeepStage) {
    Remove-Item -Path $stageRoot -Recurse -Force
}

Write-Step "MSIX package complete"
Write-Host "Package: $msixPath"
Write-Host "SHA256 : $($hash.Hash)"
Write-Host "Record : $artifactRecordPath"
if ($cerPath) {
    Write-Host "Test certificate public file: $cerPath"
}
