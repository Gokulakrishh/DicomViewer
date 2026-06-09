param(
    [Parameter(Mandatory = $true)]
    [string]$PackagePath,
    [string]$CertificatePath,
    [string]$PackageIdentityName = "CrossAxialDicomViewer",
    [switch]$TrustForLocalMachine,
    [switch]$RemoveExisting
)

$ErrorActionPreference = "Stop"

if (-not (Test-Path $PackagePath)) {
    throw "MSIX package not found: $PackagePath"
}

if ($CertificatePath) {
    if (-not (Test-Path $CertificatePath)) {
        throw "Certificate not found: $CertificatePath"
    }

    if ($TrustForLocalMachine) {
        Write-Host "Importing local test certificate into LocalMachine\TrustedPeople..."
        try {
            Import-Certificate -FilePath $CertificatePath -CertStoreLocation "Cert:\LocalMachine\TrustedPeople" | Out-Null
        }
        catch {
            throw "Unable to trust the certificate in LocalMachine\TrustedPeople. Re-run PowerShell as Administrator, or import the certificate manually with: Import-Certificate -FilePath `"$CertificatePath`" -CertStoreLocation `"Cert:\LocalMachine\TrustedPeople`""
        }
    }
    else {
        Write-Warning "Certificate trust for MSIX sideloading usually requires LocalMachine\TrustedPeople. Re-run this script as Administrator with -TrustForLocalMachine if Add-AppxPackage reports 0x800B0109."
    }
}

if ($RemoveExisting) {
    Write-Host "Removing existing installed package entries matching $PackageIdentityName..."
    Get-AppxPackage -Name $PackageIdentityName -ErrorAction SilentlyContinue |
        Remove-AppxPackage -ErrorAction SilentlyContinue
}

Write-Host "Installing MSIX package..."
Add-AppxPackage -Path $PackagePath

Write-Host "Installed package:"
Get-AppxPackage -Name $PackageIdentityName | Select-Object Name, Version, PackageFullName, InstallLocation
