Add-Type -TypeDefinition @"
using System;
using System.Collections.Generic;
public static class PeScanner3 {
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

$exePath = "D:\SteamLibrary\steamapps\common\Rocksmith\Rocksmith.exe"
$b = [System.IO.File]::ReadAllBytes($exePath)

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

function R2O([uint32]$rva) {
    for ($i = 0; $i -lt $secVA.Length; $i++) {
        if ($rva -ge $secVA[$i] -and $rva -lt ($secVA[$i] + $secVSz[$i])) {
            return [int]($secRaw[$i] + ($rva - $secVA[$i]))
        }
    }
    return -1
}
function FileOffToVA([int]$fileOff) {
    for ($i = 0; $i -lt $secVA.Length; $i++) {
        if ($fileOff -ge $secRaw[$i] -and $fileOff -lt ($secRaw[$i] + $secRawSz[$i])) {
            return $imgBase + $secVA[$i] + [uint32]($fileOff - $secRaw[$i])
        }
    }
    return [uint32]0
}
function ReadNullTerminatedAscii([int]$offset) {
    $s = ""; $j = 0
    while ($b[$offset + $j] -ne 0) { $s += [char]$b[$offset + $j]; $j++ }
    return $s
}

# --- Build a table of all IAT entry VAs for ole32.dll ---
$importDirRVA = [BitConverter]::ToUInt32($b, $peo + 24 + 104)
$importOffset = R2O $importDirRVA

$ole32Funcs = @{}  # VA -> name

$desc = 0
while ($true) {
    $d       = $importOffset + $desc * 20
    $nameRVA = [BitConverter]::ToUInt32($b, $d + 12)
    if ($nameRVA -eq 0) { break }
    $dllName = ReadNullTerminatedAscii (R2O $nameRVA)

    if ($dllName -ieq "ole32.dll") {
        $intRVA = [BitConverter]::ToUInt32($b, $d + 0)
        $iatRVA = [BitConverter]::ToUInt32($b, $d + 16)
        $intOff = R2O $intRVA
        $k = 0
        while ($true) {
            $thunkRVA = [BitConverter]::ToUInt32($b, $intOff + $k * 4)
            if ($thunkRVA -eq 0) { break }
            if (($thunkRVA -band 0x80000000) -eq 0) {
                $fnName = ReadNullTerminatedAscii ((R2O $thunkRVA) + 2)
                $iatEntryRVA = $iatRVA + [uint32]($k * 4)
                $iatEntryVA  = $imgBase + $iatEntryRVA
                $ole32Funcs["$iatEntryVA"] = $fnName
            }
            $k++
        }
        break
    }
    $desc++
}

Write-Host "ole32.dll IAT entries:"
foreach ($kv in $ole32Funcs.GetEnumerator()) {
    Write-Host ("  VA=0x{0:x8}  {1}" -f [uint32]$kv.Key, $kv.Value)
}
Write-Host ""

# --- Search for FF 25 [VA] thunks (JMP [IAT]) for each ole32 function ---
foreach ($kv in $ole32Funcs.GetEnumerator()) {
    $va = [uint32]$kv.Key
    $fn = $kv.Value
    $b0 = [byte](($va -shr  0) -band 0xFF)
    $b1 = [byte](($va -shr  8) -band 0xFF)
    $b2 = [byte](($va -shr 16) -band 0xFF)
    $b3 = [byte](($va -shr 24) -band 0xFF)

    $jmpPat = [byte[]](0xFF, 0x25, $b0, $b1, $b2, $b3)
    $jmpHits = [PeScanner3]::FindPattern($b, 0, $b.Length, $jmpPat)

    if ($jmpHits.Count -gt 0) {
        foreach ($jmpOff in $jmpHits) {
            $thunkVA = FileOffToVA $jmpOff
            Write-Host ("JMP thunk for $fn at file offset 0x{0:x8}, VA=0x{1:x8}" -f $jmpOff, $thunkVA)

            # Now search for E8 relative calls to this thunk VA
            # E8 calls use relative offset: target = (callInstrVA + 5) + rel32
            # so rel32 = target - (callInstrVA + 5)
            # For each possible code location, callInstrVA = fileOffToVA($callOff)
            # rel32 = thunkVA - (callInstrVA + 5)
            # We search: for each E8 byte, check if the next 4 bytes = rel32

            # More practical: iterate through .text looking for E8 + correct rel32
            $textIdx = -1
            for ($i = 0; $i -lt $ns; $i++) { if ($secName[$i] -eq ".text") { $textIdx = $i; break } }
            $tStart = [int]$secRaw[$textIdx]
            $tSize  = [int]$secRawSz[$textIdx]
            $tVA    = $secVA[$textIdx]

            $callHits = [System.Collections.Generic.List[int]]::new()
            for ($off = $tStart; $off -lt ($tStart + $tSize - 5); $off++) {
                if ($b[$off] -eq 0xE8) {
                    $rel32 = [BitConverter]::ToInt32($b, $off + 1)
                    $callVA = $imgBase + $tVA + [uint32]($off - $tStart)
                    $targetVA = [uint32]([int64]$callVA + 5 + $rel32)
                    if ($targetVA -eq $thunkVA) {
                        $callHits.Add($off)
                    }
                }
            }

            Write-Host ("  E8 call sites in .text: {0}" -f $callHits.Count)
            foreach ($cOff in $callHits) {
                $cVA = $imgBase + $tVA + [uint32]($cOff - $tStart)
                # Print 10 bytes: E8 + 4 rel bytes + next 4 bytes
                $arrB = for ($i = $cOff; $i -lt [Math]::Min($cOff+10, $tStart+$tSize); $i++) {
                    "0x{0:x2}" -f $b[$i]
                }
                # Also 4 bytes before, for context
                $ctxB = ""
                $ctxStart = [Math]::Max($tStart, $cOff - 8)
                for ($i = $ctxStart; $i -lt ($cOff + 10); $i++) {
                    if ($i -eq $cOff) { $ctxB += "[" }
                    $ctxB += ("{0:X2} " -f $b[$i])
                    if ($i -eq ($cOff + 4)) { $ctxB += "] " }
                }
                Write-Host ("    file=0x{0:x8}  VA=0x{1:x8}  bytes: {2}" -f $cOff, $cVA, $ctxB.Trim())
                Write-Host ("    C++: static const BYTE originalBytes[] = {{ {0} }};" -f ($arrB -join ', '))
                Write-Host ""
            }
        }
    } else {
        Write-Host ("No FF 25 thunk found for: $fn")
    }
}
