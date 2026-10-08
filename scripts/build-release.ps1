param(
    [string]$Configuration = "Release",
    [string]$Version = (Get-Content "$PSScriptRoot\..\VERSION").Trim()
)

$ErrorActionPreference = "Stop"
$Root = Resolve-Path "$PSScriptRoot\.."
$Build = Join-Path $Root "build"
$Package = Join-Path $Root "releases\package"
$Update = Join-Path $Root "releases\update"
$Zip = Join-Path $Root "releases\Overlay.zip"

cmake -S $Root -B $Build -A x64 -DOVERLAY_FETCH_DEPS=ON
cmake --build $Build --config $Configuration --parallel

dotnet publish "$Root\launcher\Launcher\Launcher.csproj" -c $Configuration -r win-x64 --self-contained true -o $Package

Copy-Item "$Build\overlay\$Configuration\Overlay.exe" "$Package\Overlay.exe" -Force
Copy-Item "$Root\overlay\assets" "$Package\assets" -Recurse -Force
Set-Content "$Package\version.txt" $Version

if (Test-Path $Update) {
    Remove-Item $Update -Recurse -Force
}
New-Item -ItemType Directory -Path $Update | Out-Null
Copy-Item "$Build\overlay\$Configuration\Overlay.exe" "$Update\Overlay.exe" -Force
Copy-Item "$Root\overlay\assets" "$Update\assets" -Recurse -Force
Set-Content "$Update\version.txt" $Version

if (Test-Path $Zip) {
    Remove-Item $Zip -Force
}

Compress-Archive -Path "$Update\*" -DestinationPath $Zip -CompressionLevel Optimal
Write-Host "Packaged $Zip"
