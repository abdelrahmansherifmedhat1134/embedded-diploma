# Builds every environment and copies each firmware.hex to hex/<env>.hex
# Run from project_v1:  powershell -File scripts/build_all_hex.ps1
$envs = @("app","test_base","test_mcal","test_hal","test_service","test_app")
New-Item -ItemType Directory -Force hex | Out-Null
foreach ($e in $envs) {
    pio run -e $e
    if ($LASTEXITCODE -ne 0) { Write-Host "BUILD FAILED: $e"; exit 1 }
    Copy-Item ".pio/build/$e/firmware.hex" "hex/$e.hex" -Force
}
Write-Host "Done. Hex files are in hex/"
