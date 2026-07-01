param(
    [string]$AdvancedPath = "src/addons/advancedPenetration.hpp",
    [string]$ApModePath = "src/addons/AP-mode.cpp"
)

Write-Host "AP-mode delta check"
Write-Host "Advanced file: $AdvancedPath"
Write-Host "AP-mode file:  $ApModePath"
Write-Host ""

if (!(Test-Path $AdvancedPath)) {
    Write-Error "Advanced file not found: $AdvancedPath"
    exit 1
}
if (!(Test-Path $ApModePath)) {
    Write-Error "AP-mode file not found: $ApModePath"
    exit 1
}

$advancedPatterns = @(
    "void parseConfig\(",
    "void parseStatus\(",
    "bool setSpeed\(",
    "bool setBaseValue\(",
    "bool setModifierValue\(",
    "void loadPresets\("
)

$apPatterns = @(
    "static void parseConfigString\(",
    "static void parseStatusString\(",
    "static bool setSpeedValue\(",
    "static bool setBaseValue\(",
    "static bool setModifierValue\(",
    "static void parsePresetsString\("
)

Write-Host "=== Upstream anchors (advancedPenetration.hpp) ==="
foreach ($p in $advancedPatterns) {
    $matches = Select-String -Path $AdvancedPath -Pattern $p
    if ($matches) {
        foreach ($m in $matches) {
            Write-Host ("[FOUND] line {0}: {1}" -f $m.LineNumber, $m.Line.Trim())
        }
    } else {
        Write-Host ("[MISS ] pattern: {0}" -f $p)
    }
}

Write-Host ""
Write-Host "=== Adapter anchors (AP-mode.cpp) ==="
foreach ($p in $apPatterns) {
    $matches = Select-String -Path $ApModePath -Pattern $p
    if ($matches) {
        foreach ($m in $matches) {
            Write-Host ("[FOUND] line {0}: {1}" -f $m.LineNumber, $m.Line.Trim())
        }
    } else {
        Write-Host ("[MISS ] pattern: {0}" -f $p)
    }
}

Write-Host ""
$hash = (Get-FileHash $AdvancedPath -Algorithm SHA256).Hash
Write-Host "Current advancedPenetration.hpp SHA256: $hash"
Write-Host ""
Write-Host "Tip: Save this hash in your release notes when AP-mode is synchronized."
