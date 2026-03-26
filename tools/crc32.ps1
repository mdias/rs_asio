Add-Type -TypeDefinition @"
using System;
using System.IO;
public class Crc32 {
    static readonly uint[] Table;
    static Crc32() {
        Table = new uint[256];
        for (uint i = 0; i < 256; i++) {
            uint c = i;
            for (int j = 0; j < 8; j++)
                c = (c & 1) != 0 ? 0xEDB88320u ^ (c >> 1) : c >> 1;
            Table[i] = c;
        }
    }
    public static uint Compute(byte[] data) {
        uint crc = 0xFFFFFFFF;
        foreach (byte b in data)
            crc = Table[(crc ^ b) & 0xFF] ^ (crc >> 8);
        return crc ^ 0xFFFFFFFF;
    }
}
"@
$bytes = [System.IO.File]::ReadAllBytes("D:\SteamLibrary\steamapps\common\Rocksmith\Rocksmith.exe")
$crc = [Crc32]::Compute($bytes)
Write-Host ("CRC32: 0x{0:x8}" -f $crc)
