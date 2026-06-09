# Windows MSIX Packaging Workflow

## Purpose

This folder contains the PowerShell-first workflow for building a Microsoft Store oriented MSIX package for Cross Axial Dicom Viewer.

MSIX is the preferred path for Microsoft Store publishing because the Store can host and deliver the package. This avoids maintaining a separate public installer hosting location.

## Current Product Boundary

The current package must be treated as a non-diagnostic demo build unless the release record, risk controls, CAPA records, and verification evidence are updated.

Do not use Store listing text that claims:

- diagnostic approval
- treatment use
- clinical decision support
- completed PACS behavior
- MDR/CDSCO/TUV approval
- cyber-proof or hospital-ready cybersecurity

## Required Windows Tools

Install on the Windows build machine:

- Visual Studio Build Tools 2022 with MSVC x64
- Windows 10/11 SDK, including `makeappx.exe` and `signtool.exe`
- CMake
- Ninja, unless using the Visual Studio generator
- Qt 6 MSVC 2022 x64
- VTK built with the same MSVC/Qt runtime
- GDCM built with the same MSVC runtime
- Windows App Certification Kit for pre-submission testing

All Qt, VTK, and GDCM binaries must use the same architecture and runtime family. Do not mix Debug GDCM with Release Qt/app binaries.

## Recommended Folder Assumptions

Example dependency layout:

```text
C:\Qt\6.6.3\msvc2022_64
C:\VTK\install\lib\cmake\vtk-9.3
C:\VTK\install\bin
C:\GDCM\install\lib\cmake\gdcm
C:\GDCM\install\bin
```

Adjust paths to match the actual Windows machine.

## Create A Local Test MSIX

From a PowerShell terminal in the repository root:

```powershell
$password = Read-Host -AsSecureString "MSIX test certificate password"

.\packaging\windows\msix\Package-WindowsMsix.ps1 `
  -QtRoot "C:\Qt\6.6.3\msvc2022_64" `
  -VTK_DIR "C:\VTK\install\lib\cmake\vtk-9.3" `
  -VtkBin "C:\VTK\install\bin" `
  -GDCM_DIR "C:\GDCM\install\lib\cmake\gdcm" `
  -GdcmBin "C:\GDCM\install\bin" `
  -CreateTestCertificate `
  -CertificatePassword $password
```

The script will:

1. configure CMake with VTK enabled
2. build `CrossAxialDicomViewer`
3. stage the executable
4. run `windeployqt`
5. copy VTK/GDCM DLLs from supplied bin folders
6. generate MSIX visual assets from the official app icon
7. generate `AppxManifest.xml`
8. create the `.msix` with `makeappx`
9. sign it with a local test certificate if requested
10. write a SHA256 artifact record

Generated output is written to:

```text
artifacts\windows-msix\
```

## Install A Local Test MSIX

```powershell
.\packaging\windows\msix\Install-TestMsix.ps1 `
  -PackagePath ".\artifacts\windows-msix\CrossAxialDicomViewer-1.0.0.1-x64.msix" `
  -CertificatePath ".\artifacts\windows-msix\certs\CrossAxialDicomViewer-TestCertificate.cer" `
  -TrustForLocalMachine `
  -RemoveExisting
```

Run this installation command from an elevated PowerShell terminal because `-TrustForLocalMachine` imports the local test certificate into `LocalMachine\TrustedPeople`.

If you rebuild with the same version, use `-RemoveExisting` or increment the build number in `CMakeLists.txt`.

The helper can import the local self-signed test certificate into the machine's `TrustedPeople` certificate store for sideload testing. Do not use the test certificate as a production Microsoft Store signing identity.

## Microsoft Store Notes

For Store submission, reserve/create the app in Partner Center first. Then use the exact package identity and publisher values from Partner Center.

Example:

```powershell
$password = Read-Host -AsSecureString "Signing certificate password"

.\packaging\windows\msix\Package-WindowsMsix.ps1 `
  -QtRoot "C:\Qt\6.6.3\msvc2022_64" `
  -VTK_DIR "C:\VTK\install\lib\cmake\vtk-9.3" `
  -VtkBin "C:\VTK\install\bin" `
  -GDCM_DIR "C:\GDCM\install\lib\cmake\gdcm" `
  -GdcmBin "C:\GDCM\install\bin" `
  -PackageIdentityName "PartnerCenter.Package.Identity.Name" `
  -PublisherName "CN=PublisherNameFromPartnerCenter" `
  -PublisherDisplayName "Your Publisher Display Name" `
  -CertificatePath "C:\certs\store-signing-cert.pfx" `
  -CertificatePassword $password
```

Do not upload a locally self-signed test package as a final Store package unless Partner Center explicitly accepts that signing route for the chosen submission workflow.

## Validation Checklist

Before Store upload or external distribution:

- package installs on a clean Windows 11 machine
- app launches from Start menu
- splash/non-diagnostic notice appears
- DICOM folder import works through file/folder picker
- app writes SQLite/log/audit files under the user-writable application data path, not the install folder
- VTK viewer opens without missing DLL errors
- GDCM decoding works on representative CT/MR/XA files
- corrupted/malformed DICOM limitation is documented until CAPA-001 closes
- measurement/MPR clinical correctness limitation is documented until CAPA-002 closes
- Windows App Certification Kit passes or failures are recorded
- SHA256 and artifact record are stored in the release evidence

## Troubleshooting

### Missing DLLs

Add the dependency bin folder through `-VtkBin`, `-GdcmBin`, or `-ExtraBinDirs`.

If `msvcp140.dll`, `msvcp140_1.dll`, `msvcp140_2.dll`, `vcruntime140.dll`, `vcruntime140_1.dll`, or `concrt140.dll` are missing, install the Microsoft Visual C++ 2015-2022 x64 Redistributable or pass the runtime DLL folder explicitly:

```powershell
-MsvcRuntimeDir "C:\Program Files\Microsoft Visual Studio\2022\BuildTools\VC\Redist\MSVC\<version>\x64\Microsoft.VC143.CRT"
```

The packaging script copies these MSVC runtime DLLs explicitly because `windeployqt --compiler-runtime` may not copy them on every Windows setup.

### `_ITERATOR_DEBUG_LEVEL` Or Runtime Library Mismatch

Rebuild the app and dependencies with the same MSVC runtime and configuration. Do not link Debug GDCM/VTK libraries into a Release app.

### `build.ninja` Not Found

This means CMake configure did not complete, so Ninja had no generated build file to run. Check the first error printed under `Configuring CMake`. The usual causes are an incorrect `QtRoot`, `VTK_DIR`, or `GDCM_DIR`, or Ninja not being available in `PATH`.

If Ninja is not installed, either install Ninja or run the packaging script with:

```powershell
-Generator "Visual Studio 17 2022" `
-Architecture "x64"
```

### App Cannot Write Settings Or Database

The app must not write into the MSIX install folder. Current application paths use the user's Documents folder under the product name; verify this on Windows after install.

### Store Package Identity Error

Use the package identity and publisher string from Partner Center. The local development defaults are only for test packages.

### Certificate Not Trusted: `0x800B0109`

Run PowerShell as Administrator and import the test certificate into the local machine trust store:

```powershell
Import-Certificate `
  -FilePath ".\artifacts\windows-msix\certs\CrossAxialDicomViewer-TestCertificate.cer" `
  -CertStoreLocation "Cert:\LocalMachine\TrustedPeople"
```

Then rerun `Install-TestMsix.ps1` with `-TrustForLocalMachine`.

### Script Execution Is Disabled

If `.ps1` execution is blocked by the machine owner, run the install steps manually from an elevated PowerShell terminal instead of running `Install-TestMsix.ps1`:

```powershell
Import-Certificate `
  -FilePath ".\artifacts\windows-msix\certs\CrossAxialDicomViewer-TestCertificate.cer" `
  -CertStoreLocation "Cert:\LocalMachine\TrustedPeople"

Get-AppxPackage -Name "CrossAxialDicomViewer" -ErrorAction SilentlyContinue | Remove-AppxPackage -ErrorAction SilentlyContinue

Add-AppxPackage -Path ".\artifacts\windows-msix\CrossAxialDicomViewer-1.0.0.1-x64.msix"
```

If administrator rights are also blocked, a self-signed MSIX cannot be sideloaded on that machine. Use a Store-signed package, ask the owner/admin to trust the test certificate, or test the unpackaged portable build instead.
