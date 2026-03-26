Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
public static class PeScanner2 {
    public static List<int> FindPattern(byte[] data, int start, int length, byte[] pattern) {
        var r = new List<int>();
        int end = start + length - pattern.Length;
        for (int i = start; i <= end; i++) {
            bool m = true;
            for (int j = 0; j < pattern.Length; j++) {
                if (data[i + j] != pattern[j]) { m = false; break; }
            }
            if (m) r.Add(i);
        }
        return r;
    }
}
"@

$b = [System.IO.File]::ReadAllBytes("D:\SteamLibrary\steamapps\common\Rocksmith\Rocksmith.exe")

# Get section table
$peo  = [BitConverter]::ToInt32($b, 0x3C)
$ns   = [BitConverter]::ToUInt16($b, $peo + 6)
$soh  = [BitConverter]::ToUInt16($b, $peo + 20)
$sto  = $peo + 24 + $soh

Write-Host "Section raw sizes:"
for ($i = 0; $i -lt $ns; $i++) {
    $s     = $sto + $i * 40
    $nm    = [System.Text.Encoding]::ASCII.GetString($b, $s, 8).TrimEnd([char]0)
    $vsz   = [BitConverter]::ToUInt32($b, $s + 8)
    $rawsz = [BitConverter]::ToUInt32($b, $s + 16)
    Write-Host ("  [{0,-8}] VirtualSize=0x{1:X8}  SizeOfRawData=0x{2:X8}" -f $nm, $vsz, $rawsz)
}

Write-Host ""

# Search entire file for the CoCreateInstance IAT call: FF 15 7C D4 C8 00
$pat  = [byte[]](0xFF, 0x15, 0x7C, 0xD4, 0xC8, 0x00)
$hits = [PeScanner2]::FindPattern($b, 0, $b.Length, $pat)
Write-Host ("FF 15 7C D4 C8 00 in entire file: {0} hit(s)" -f $hits.Count)
foreach ($off in $hits) {
    Write-Host ("  file offset 0x{0:x8}" -f $off)
}

Write-Host ""

# Also check if .text VirtualSize vs SizeOfRawData differ significantly
# (would indicate packing)
$textS   = $sto  # first section is .text
$textVSz = [BitConverter]::ToUInt32($b, $textS + 8)
$textRSz = [BitConverter]::ToUInt32($b, $textS + 16)
if ($textRSz -lt ($textVSz * 0.5)) {
    Write-Host ".text SizeOfRawData is much smaller than VirtualSize -> section is likely packed."
} else {
    Write-Host ".text SizeOfRawData roughly matches VirtualSize -> section is probably not packed."
}

# Check PSFD00 section flags (characteristics at offset +36)
Write-Host ""
for ($i = 0; $i -lt $ns; $i++) {
    $s    = $sto + $i * 40
    $nm   = [System.Text.Encoding]::ASCII.GetString($b, $s, 8).TrimEnd([char]0)
    $char = [BitConverter]::ToUInt32($b, $s + 36)
    $exec = ($char -band 0x20000000) -ne 0
    $read = ($char -band 0x40000000) -ne 0
    $writ = ($char -band 0x80000000) -ne 0
    $code = ($char -band 0x00000020) -ne 0
    Write-Host ("  [{0,-8}] flags=0x{1:X8}  code={2}  exec={3}  read={4}  write={5}" -f $nm, $char, $code, $exec, $read, $writ)
}
