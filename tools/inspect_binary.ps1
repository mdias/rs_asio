$b = [System.IO.File]::ReadAllBytes("D:\SteamLibrary\steamapps\common\Rocksmith\Rocksmith.exe")

$peo  = [BitConverter]::ToInt32($b, 0x3C)
$ns   = [BitConverter]::ToUInt16($b, $peo + 6)
$soh  = [BitConverter]::ToUInt16($b, $peo + 20)
$sto  = $peo + 24 + $soh
$imgBase = [BitConverter]::ToUInt32($b, $peo + 24 + 28)

$secName  = [string[]]::new($ns); $secVA = [uint32[]]::new($ns)
$secVSz   = [uint32[]]::new($ns); $secRaw = [uint32[]]::new($ns)
$secRawSz = [uint32[]]::new($ns)
for ($i = 0; $i -lt $ns; $i++) {
    $s = $sto + $i * 40
    $secName[$i]  = [System.Text.Encoding]::ASCII.GetString($b, $s, 8).TrimEnd([char]0)
    $secVSz[$i]   = [BitConverter]::ToUInt32($b, $s + 8)
    $secVA[$i]    = [BitConverter]::ToUInt32($b, $s + 12)
    $secRawSz[$i] = [BitConverter]::ToUInt32($b, $s + 16)
    $secRaw[$i]   = [BitConverter]::ToUInt32($b, $s + 20)
}

# Check data directories beyond import (index 0..15, each 8 bytes at optHeaderOffset+96)
Write-Host "Data directories:"
$ddBase = $peo + 24 + 96
$ddNames = @("Export","Import","Resource","Exception","Security","BaseReloc","Debug",
             "Architecture","GlobalPtr","TLS","LoadConfig","BoundImport","IAT",
             "DelayImport","COMDesc","Reserved")
for ($i = 0; $i -lt 16; $i++) {
    $rva  = [BitConverter]::ToUInt32($b, $ddBase + $i * 8)
    $size = [BitConverter]::ToUInt32($b, $ddBase + $i * 8 + 4)
    if ($rva -ne 0) {
        Write-Host ("  [{0,12}] RVA=0x{1:x8}  Size=0x{2:x8}" -f $ddNames[$i], $rva, $size)
    }
}

Write-Host ""

# Dump first 128 bytes of .text as hex to verify it looks like real x86
$textRaw = $secRaw[0]
Write-Host "First 128 bytes of .text section (file offset 0x$(('{0:x}' -f $textRaw))):"
$line = ""
for ($i = 0; $i -lt 128; $i++) {
    $line += ("{0:X2} " -f $b[$textRaw + $i])
    if (($i+1) % 16 -eq 0) { Write-Host "  $line"; $line = "" }
}

Write-Host ""

# Count FF 15, FF 25, E8, E9 instructions in .text for sanity check
$ff15 = 0; $ff25 = 0; $e8 = 0; $e9 = 0
$tStart = [int]$secRaw[0]; $tSize = [int]$secRawSz[0]
for ($i = $tStart; $i -lt ($tStart + $tSize - 1); $i++) {
    if ($b[$i] -eq 0xFF) {
        if ($b[$i+1] -eq 0x15) { $ff15++ }
        if ($b[$i+1] -eq 0x25) { $ff25++ }
    }
    if ($b[$i] -eq 0xE8) { $e8++ }
    if ($b[$i] -eq 0xE9) { $e9++ }
}
Write-Host ".text instruction counts (approximate):"
Write-Host "  FF 15 (CALL [mem]):  $ff15"
Write-Host "  FF 25 (JMP  [mem]):  $ff25"
Write-Host "  E8    (CALL rel):    $e8"
Write-Host "  E9    (JMP  rel):    $e9"

Write-Host ""

# Show first 64 bytes of .bind section
$bindIdx = -1
for ($i = 0; $i -lt $ns; $i++) { if ($secName[$i] -eq ".bind") { $bindIdx = $i; break } }
if ($bindIdx -ge 0) {
    Write-Host ("First 64 bytes of .bind section (file offset 0x{0:x8}):" -f $secRaw[$bindIdx])
    $line = ""
    for ($i = 0; $i -lt 64; $i++) {
        $line += ("{0:X2} " -f $b[$secRaw[$bindIdx] + $i])
        if (($i+1) % 16 -eq 0) { Write-Host "  $line"; $line = "" }
    }
}
