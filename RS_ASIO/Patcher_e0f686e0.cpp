#include "stdafx.h"
#include "dllmain.h"
#include "Patcher.h"

// Patch code for Rocksmith (2011), CRC32 0xe0f686e0
//
// The Rocksmith 2011 executable encrypts its .text section on disk using a custom
// packer stub (PSFD00 section), so call-site byte patterns cannot be found statically.
// Instead, we patch the Import Address Table (IAT) directly.
//
// The IAT is in .rdata (unencrypted) and is filled by the loader before our DLL runs.
// We overwrite the function pointer slots for the COM functions that PortAudio/WASAPI
// uses to enumerate audio devices, replacing them with our own implementations.
//
// IAT entry RVAs (relative to image base 0x00400000, verified from the PE headers):
//
//   CoCreateInstance                      RVA 0x0088d47c  (ole32.dll)
//   CoMarshalInterThreadInterfaceInStream  RVA 0x0088d490  (ole32.dll)
//   CoGetInterfaceAndReleaseStream         RVA 0x0088d494  (ole32.dll)

// Known IAT RVAs - these are fixed offsets from the module base and do not change
// because the executable does not use ASLR (ImageBase is fixed at 0x00400000).
static const DWORD IAT_RVA_CoCreateInstance                     = 0x0088d47c;
static const DWORD IAT_RVA_CoMarshalInterThreadInterfaceInStream = 0x0088d490;
static const DWORD IAT_RVA_CoGetInterfaceAndReleaseStream        = 0x0088d494;

// Patch a single IAT slot to point to replacementFn.
// Patch_ReplaceWithBytes (from Patcher.h) handles page protection internally
// via NtProtectVirtualMemory, bypassing any hook on VirtualProtect.
static bool PatchIATEntry(DWORD rva, void* replacementFn, const char* fnName)
{
	const HMODULE hBase = GetModuleHandle(NULL);
	if (!hBase)
	{
		rslog::error_ts() << "PatchIATEntry: GetModuleHandle failed for " << fnName << std::endl;
		return false;
	}

	void** iatSlot = reinterpret_cast<void**>(reinterpret_cast<BYTE*>(hBase) + rva);

	rslog::info_ts() << "PatchIATEntry: patching " << fnName
	                 << " at " << iatSlot
	                 << "  old=" << *iatSlot
	                 << "  new=" << replacementFn << std::endl;

	Patch_ReplaceWithBytes(iatSlot, sizeof(void*), reinterpret_cast<const BYTE*>(&replacementFn));
	return true;
}

// Thin wrappers that match the calling convention of the ole32 functions we are replacing.
// CoMarshalInterThreadInterfaceInStream / CoGetInterfaceAndReleaseStream are patched to
// no-op / identity replacements so that PortAudio's cross-thread COM marshaling does not
// interfere with our fake WASAPI device objects.
//
// NOTE: These are only used if RS2011's embedded PortAudio performs cross-apartment
// marshaling the same way RS2014's does. The game may work without them; if it crashes
// after CoCreateInstance is redirected successfully, these patches are the next step.

static HRESULT STDAPICALLTYPE Patched_CoMarshalInterThreadInterfaceInStream(
    REFIID riid, IUnknown* pUnk, IStream** ppStm)
{
	rslog::info_ts() << "Patched_CoMarshalInterThreadInterfaceInStream called" << std::endl;
	// Return the interface directly as the "stream" — PortAudio will read it back
	// via Patched_CoGetInterfaceAndReleaseStream below.
	if (!ppStm)
		return E_POINTER;
	*ppStm = reinterpret_cast<IStream*>(pUnk);
	if (pUnk)
		pUnk->AddRef();
	return S_OK;
}

static HRESULT STDAPICALLTYPE Patched_CoGetInterfaceAndReleaseStream(
    IStream* pStm, REFIID riid, void** ppv)
{
	rslog::info_ts() << "Patched_CoGetInterfaceAndReleaseStream called" << std::endl;
	if (!ppv)
		return E_POINTER;
	// The "stream" is actually the interface pointer we stored above.
	IUnknown* pUnk = reinterpret_cast<IUnknown*>(pStm);
	if (!pUnk)
	{
		*ppv = nullptr;
		return E_NOINTERFACE;
	}
	HRESULT hr = pUnk->QueryInterface(riid, ppv);
	pUnk->Release(); // CoGetInterfaceAndReleaseStream always releases the stream
	return hr;
}

void PatchOriginalCode_e0f686e0()
{
	rslog::info_ts() << __FUNCTION__ << " - patching Rocksmith 2011 via IAT" << std::endl;

	bool ok = true;

	ok &= PatchIATEntry(IAT_RVA_CoCreateInstance,
	                    reinterpret_cast<void*>(&Patched_CoCreateInstance),
	                    "CoCreateInstance");

	ok &= PatchIATEntry(IAT_RVA_CoMarshalInterThreadInterfaceInStream,
	                    reinterpret_cast<void*>(&Patched_CoMarshalInterThreadInterfaceInStream),
	                    "CoMarshalInterThreadInterfaceInStream");

	ok &= PatchIATEntry(IAT_RVA_CoGetInterfaceAndReleaseStream,
	                    reinterpret_cast<void*>(&Patched_CoGetInterfaceAndReleaseStream),
	                    "CoGetInterfaceAndReleaseStream");

	if (!ok)
	{
		rslog::error_ts() << __FUNCTION__ << " - one or more IAT patches failed" << std::endl;
	}
	else
	{
		rslog::info_ts() << __FUNCTION__ << " - all IAT patches applied successfully" << std::endl;
	}
}
