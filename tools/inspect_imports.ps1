$exe = "D:\SteamLibrary\steamapps\common\Rocksmith\Rocksmith.exe"
$bytes = [System.IO.File]::ReadAllBytes($exe)

$peOffset        = [BitConverter]::ToInt32($bytes, 0x3C)
$numSections     = [BitConverter]::ToUInt16($bytes, $peOffset + 6)
$sizeOfOptHeader = [BitConverter]::ToUInt16($bytes, $peOffset + 20)
$optHeaderOffset = $peOffset + 24
$secTableOffset  = $optHeaderOffset + $sizeOfOptHeader
$importDirRVA    = [BitConverter]::ToUInt32($bytes, $optHeaderOffset + 104)

$secVA  = [uint32[]]::new($numSections)
$secVSz = [uint32[]]::new($numSections)
$secRaw = [uint32[]]::new($numSections)
for ($i = 0; $i -lt $numSections; $i++) {
    $s          = $secTableOffset + $i * 40
    $secVSz[$i] = [BitConverter]::ToUInt32($bytes, $s + 8)
    $secVA[$i]  = [BitConverter]::ToUInt32($bytes, $s + 12)
    $secRaw[$i] = [BitConverter]::ToUInt32($bytes, $s + 20)
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
    $s = ""
    $j = 0
    while ($bytes[$offset + $j] -ne 0) {
        $s += [char]$bytes[$offset + $j]
        $j++
    }
    return $s
}

$importOffset = R2O $importDirRVA
Write-Host "=== All imported DLLs and their functions ==="
$i = 0
while ($true) {
    $d       = $importOffset + $i * 20
    $nameRVA = [BitConverter]::ToUInt32($bytes, $d + 12)
    if ($nameRVA -eq 0) { break }

    $nameOff = R2O $nameRVA
    $dllName = ReadNullTerminatedAscii $nameOff

    Write-Host ""
    Write-Host "  [$dllName]"

    $intRVA = [BitConverter]::ToUInt32($bytes, $d + 0)
    if ($intRVA -ne 0) {
        $intOff = R2O $intRVA
        $k = 0
        while ($true) {
            $thunkRVA = [BitConverter]::ToUInt32($bytes, $intOff + $k * 4)
            if ($thunkRVA -eq 0) { break }
            if (($thunkRVA -band 0x80000000) -eq 0) {
                $hintNameOff = R2O $thunkRVA
                $fnName = ReadNullTerminatedAscii ($hintNameOff + 2)
                Write-Host "    $fnName"
            } else {
                Write-Host "    (ordinal $($thunkRVA -band 0x7FFFFFFF))"
            }
            $k++
        }
    }
    $i++
}
