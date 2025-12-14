// This forces instantiation of the GUIDS
#include <initguid.h>

#include "stdafx.h"

#ifdef __GNUC__
#include <unknwn.h>
__CRT_UUID_DECL(IUnknown, 0x00000000, 0x0000, 0x0000, 0xc0,0x00, 0x00,0x00,0x00,0x00,0x00,0x46)
#endif

#pragma comment(lib, "propsys.lib")
