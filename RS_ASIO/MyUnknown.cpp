#include "MyUnknown.h"

#ifdef __GNUC__
// This changes DEFINE_PROPERTYKEY to declare (on top of defined)
#include <initguid.h>

// {F2D67F48-1977-4991-A3FC-A093835A7DC2}
DEFINE_GUID(IID_IMyUnknown , 0xf2d67f48, 0x1977, 0x4991, 0xa3, 0xfc, 0xa0, 0x93, 0x83, 0x5a, 0x7d, 0xc2);

template<> struct __wine_uuidof<class MyUnknown > {
    static GUID uuid;
};
GUID __wine_uuidof<MyUnknown>::uuid = IID_IMyUnknown;
#endif
