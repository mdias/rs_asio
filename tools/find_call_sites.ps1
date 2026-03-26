# find_call_sites.ps1
# Finds all call sites for a named imported function in a PE32 executable.
# Prints the surrounding byte patterns suitable for use in RS-ASIO patcher files.

param(
    [string]$ExePath = "D:\SteamLibrary\steamapps\common\Rocksmith\Rocksmith.exe",
    [string]$FunctionName = "CoCreateInstance",
    [int]$ContextBytes = 12   # bytes to show before and after the call
)

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

# --- Find the IAT entry RVA for the target function ---
$iatEntryRVA = [uint32]0
$importOffset = R2O $importDirRVA
$desc = 0
while ($true) {
    $d       = $importOffset + $desc * 20
    $nameRVA = [BitConverter]::ToUInt32($bytes, $d + 12)
    if ($nameRVA -eq 0) { break }

    $iatRVA  = [BitConverter]::ToUInt32($bytes, $d + 16)  # FirstThunk RVA (= IAT on disk)
    $intRVA  = [BitConverter]::ToUInt32($bytes, $d + 0)   # OriginalFirstThunk RVA

    $intOff = R2O $intRVA
    $iatOff = R2O $iatRVA

    $k = 0
    while ($true) {
        $thunkRVA = [BitConverter]::ToUInt32($bytes, $intOff + $k * 4)
        if ($thunkRVA -eq 0) { break }
        if (($thunkRVA -band 0x80000000) -eq 0) {
            $fnName = ReadNullTerminatedAscii ((R2O $thunkRVA) + 2)
            if ($fnName -eq $FunctionName) {
                $iatEntryRVA = $iatRVA + [uint32]($k * 4)
                $iatEntryVA  = $imageBase + $iatEntryRVA
                Write-Host "Found '$FunctionName' IAT entry:"
                Write-Host ("  RVA    = 0x{0:x8}" -f $iatEntryRVA)
                Write-Host ("  VA     = 0x{0:x8}  (ImageBase 0x{1:x8} + RVA)" -f $iatEntryVA, $imageBase)
                Write-Host ("  FileOff= 0x{0:x8}" -f (R2O $iatEntryRVA))
                break
            }
        }
        $k++
    }
    if ($iatEntryRVA -ne 0) { break }
    $desc++
}

if ($iatEntryRVA -eq 0) {
    Write-Host "Function '$FunctionName' not found in import table."
    exit 1
}

# --- Search .text section for:  FF 15 [iatEntryVA little-endian 4 bytes] ---
$iatEntryVA = $imageBase + $iatEntryRVA
$b0 = [byte](($iatEntryVA -shr  0) -band 0xFF)
$b1 = [byte](($iatEntryVA -shr  8) -band 0xFF)
$b2 = [byte](($iatEntryVA -shr 16) -band 0xFF)
$b3 = [byte](($iatEntryVA -shr 24) -band 0xFF)

Write-Host ""
Write-Host ("Searching for call sites: FF 15 {0:X2} {1:X2} {2:X2} {3:X2}" -f $b0,$b1,$b2,$b3)
Write-Host ""

# Search the .text section only
$textIdx = -1
for ($i = 0; $i -lt $numSections; $i++) {
    if ($secName[$i] -eq ".text") { $textIdx = $i; break }
}
if ($textIdx -lt 0) { Write-Host ".text section not found"; exit 1 }

$textStart = [int]$secRaw[$textIdx]
$textSize  = [int]$secRawSz[$textIdx]
$textVA    = $secVA[$textIdx]

$found = 0
for ($off = $textStart; $off -lt ($textStart + $textSize - 6); $off++) {
    if ($bytes[$off]   -eq 0xFF -and
        $bytes[$off+1] -eq 0x15 -and
        $bytes[$off+2] -eq $b0  -and
        $bytes[$off+3] -eq $b1  -and
        $bytes[$off+4] -eq $b2  -and
        $bytes[$off+5] -eq $b3) {

        $found++
        $rva = $textVA + ($off - $textStart)
        Write-Host ("=== Call site #{0}  (file offset 0x{1:x8}, RVA 0x{2:x8}, VA 0x{3:x8}) ===" -f $found, $off, $rva, ($imageBase + $rva))

        # Print context bytes
        $start = [Math]::Max($textStart, $off - $ContextBytes)
        $end   = [Math]::Min($textStart + $textSize - 1, $off + 5 + $ContextBytes)

        $hexLine = ""
        for ($i = $start; $i -le $end; $i++) {
            if ($i -eq $off) { $hexLine += "[" }
            $hexLine += ("{0:X2}" -f $bytes[$i])
            if ($i -eq ($off + 5)) { $hexLine += "]" }
            $hexLine += " "
        }
        Write-Host "  Bytes (call site in brackets):"
        Write-Host "  $hexLine"
        Write-Host ""

        # Print as C++ byte array (6 bytes of the call + 4 bytes after = 10 bytes)
        $arrBytes = @()
        for ($i = $off; $i -lt [Math]::Min($off + 10, $textStart + $textSize); $i++) {
            $arrBytes += ("0x{0:x2}" -f $bytes[$i])
        }
        Write-Host "  C++ snippet (call + 4 bytes after):"
        Write-Host "  static const BYTE originalBytes_call_$FunctionName`[] = {"
        Write-Host "      $($arrBytes -join ', ')"
        Write-Host "  };"
        Write-Host ""
    }
}

if ($found -eq 0) {
    Write-Host "No FF 15 call sites found. The call may be a relative CALL (E8) via a thunk."
    Write-Host "Searching for E8 thunk pattern..."
    # TODO: extend for E8 relative calls if needed
}

Write-Host "Total call sites found: $found"
