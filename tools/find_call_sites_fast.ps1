# find_call_sites_fast.ps1
# Fast version using compiled C# for the inner search loop.

param(
    [string]$ExePath = "D:\SteamLibrary\steamapps\common\Rocksmith\Rocksmith.exe",
    [string]$FunctionName = "CoCreateInstance",
    [int]$ContextBytes = 12
)

Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
public static class PeScanner {
    public static List<int> FindPattern(byte[] data, int start, int length, byte[] pattern) {
        var results = new List<int>();
        int end = start + length - pattern.Length;
        for (int i = start; i <= end; i++) {
            bool match = true;
            for (int j = 0; j < pattern.Length; j++) {
                if (data[i + j] != pattern[j]) { match = false; break; }
            }
            if (match) results.Add(i);
        }
        return results;
    }
}
"@

$bytes = [System.IO.File]::ReadAllBytes($ExePath)

$peOffset        = [BitConverter]::ToInt32($bytes, 0x3C)
$numSections     = [BitConverter]::ToUInt16($bytes, $peOffset + 6)
$sizeOfOptHeader = [BitConverter]::ToUInt16($bytes, $peOffset + 20)
$optHeaderOffset = $peOffset + 24
$imageBase       = [BitConverter]::ToUInt32($bytes, $optHeaderOffset + 28)
$secTableOffset  = $optHeaderOffset + $sizeOfOptHeader
$importDirRVA    = [BitConverter]::ToUInt32($bytes, $optHeaderOffset + 104)

$secName  = [string[]]::new($numSections)
$secVA    = [uint32[]]::new($numSections)
$secVSz   = [uint32[]]::new($numSections)
$secRaw   = [uint32[]]::new($numSections)
$secRawSz = [uint32[]]::new($numSections)
for ($i = 0; $i -lt $numSections; $i++) {
    $s            = $secTableOffset + $i * 40
    $secName[$i]  = [System.Text.Encoding]::ASCII.GetString($bytes, $s, 8).TrimEnd([char]0)
    $secVSz[$i]   = [BitConverter]::ToUInt32($bytes, $s + 8)
    $secVA[$i]    = [BitConverter]::ToUInt32($bytes, $s + 12)
    $secRawSz[$i] = [BitConverter]::ToUInt32($bytes, $s + 16)
    $secRaw[$i]   = [BitConverter]::ToUInt32($bytes, $s + 20)
}

function R2O([uint32]$rva) {
    for ($i = 0; $i -lt $secVA.Length; $i++) {
        if ($rva -ge $secVA[$i] -and $rva -lt ($secVA[$i] + $secVSz[$i])) {
            return [int]($secRaw[$i] + ($rva - $secVA[$i]))
        }
    }
    return -1
}

function ReadNullTerminatedAscii([int]$offset) {
    $s = ""; $j = 0
    while ($bytes[$offset + $j] -ne 0) { $s += [char]$bytes[$offset + $j]; $j++ }
    return $s
}

# Find IAT entry RVA for target function
$iatEntryRVA = [uint32]0
$importOffset = R2O $importDirRVA
$desc = 0
while ($true) {
    $d       = $importOffset + $desc * 20
    $nameRVA = [BitConverter]::ToUInt32($bytes, $d + 12)
    if ($nameRVA -eq 0) { break }
    $intRVA = [BitConverter]::ToUInt32($bytes, $d + 0)
    $iatRVA = [BitConverter]::ToUInt32($bytes, $d + 16)
    $intOff = R2O $intRVA
    $k = 0
    while ($true) {
        $thunkRVA = [BitConverter]::ToUInt32($bytes, $intOff + $k * 4)
        if ($thunkRVA -eq 0) { break }
        if (($thunkRVA -band 0x80000000) -eq 0) {
            $fnName = ReadNullTerminatedAscii ((R2O $thunkRVA) + 2)
            if ($fnName -eq $FunctionName) {
                $iatEntryRVA = $iatRVA + [uint32]($k * 4)
                break
            }
        }
        $k++
    }
    if ($iatEntryRVA -ne 0) { break }
    $desc++
}

if ($iatEntryRVA -eq 0) { Write-Host "Function not found in import table."; exit 1 }

$iatEntryVA = $imageBase + $iatEntryRVA
Write-Host ("Found '$FunctionName' IAT entry: VA=0x{0:x8}" -f $iatEntryVA)

$b0 = [byte](($iatEntryVA -shr  0) -band 0xFF)
$b1 = [byte](($iatEntryVA -shr  8) -band 0xFF)
$b2 = [byte](($iatEntryVA -shr 16) -band 0xFF)
$b3 = [byte](($iatEntryVA -shr 24) -band 0xFF)

$callPattern = [byte[]](0xFF, 0x15, $b0, $b1, $b2, $b3)
Write-Host ("Searching for: FF 15 {0:X2} {1:X2} {2:X2} {3:X2}" -f $b0,$b1,$b2,$b3)

$textIdx = -1
for ($i = 0; $i -lt $numSections; $i++) {
    if ($secName[$i] -eq ".text") { $textIdx = $i; break }
}
if ($textIdx -lt 0) { Write-Host ".text section not found"; exit 1 }

$textStart = [int]$secRaw[$textIdx]
$textSize  = [int]$secRawSz[$textIdx]
$textVA    = $secVA[$textIdx]

$matches = [PeScanner]::FindPattern($bytes, $textStart, $textSize, $callPattern)

Write-Host ("Found {0} call site(s)." -f $matches.Count)
Write-Host ""

$n = 0
foreach ($off in $matches) {
    $n++
    $rva = [uint32]$textVA + [uint32]($off - $textStart)
    $va  = $imageBase + $rva
    Write-Host ("=== Call site #$n  (file offset 0x{0:x8}, RVA 0x{1:x8}) ===" -f $off, $rva)

    $start = [Math]::Max($textStart, $off - $ContextBytes)
    $end   = [Math]::Min($textStart + $textSize - 1, $off + 5 + $ContextBytes)
    $hexLine = ""
    for ($i = $start; $i -le $end; $i++) {
        if ($i -eq $off)       { $hexLine += "[" }
        $hexLine += ("{0:X2}" -f $bytes[$i])
        if ($i -eq ($off + 5)) { $hexLine += "]" }
        $hexLine += " "
    }
    Write-Host "  Context (call in brackets):"
    Write-Host "  $hexLine"

    # C++ array: 6 bytes of the CALL + next 4 bytes
    $arrBytes = for ($i = $off; $i -lt [Math]::Min($off + 10, $textStart + $textSize); $i++) {
        "0x{0:x2}" -f $bytes[$i]
    }
    Write-Host ""
    Write-Host "  C++ byte array (10 bytes starting at the CALL):"
    Write-Host "  static const BYTE originalBytes_call_$FunctionName`[] = {"
    Write-Host "      $($arrBytes -join ', ')"
    Write-Host "  };"
    Write-Host ""
}
