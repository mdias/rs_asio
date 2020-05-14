#pragma once

#include "ComBaseUnknown.h"

// {F2D67F48-1977-4991-A3FC-A093835A7DC2}
DEFINE_GUID(IID_IMyUnknown , 0xf2d67f48, 0x1977, 0x4991, 0xa3, 0xfc, 0xa0, 0x93, 0x83, 0x5a, 0x7d, 0xc2);

class __declspec(uuid("F2D67F48-1977-4991-A3FC-A093835A7DC2")) MyUnknown : public ComBaseUnknown<IUnknown>
{
public:
	bool IsAsio4All = false;
};
