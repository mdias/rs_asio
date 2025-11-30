#pragma once

#include "function_traits.h"

#if _WIN32
template<typename TFunc>
class TrampolineToMethod
{
private:
	using TFuncRetType = typename function_traits<TFunc>::result_type;

public:
	template<typename TClass, typename TMemberFunc>
	TrampolineToMethod(TClass& obj, TMemberFunc fn)
	{
		static void* memberFnPtr = *(void**)&fn;
		static void* redirectorFnPtr = GenerateFuncPtr<TClass, TMemberFunc, function_traits<TFunc>::arity>(&obj);;

		const BYTE opCodes[]{
			0x89, 0x44, 0x24, 0xf4,             // mov DWORD PTR [esp-12], eax (backup eax value)
			0x58,                               // pop eax (pop return address into eax)
			0x68, 0xff, 0xff, 0xff, 0xff,       // push fn pointer
			0x68, 0xff, 0xff, 0xff, 0xff,       // push this pointer
			0x50,                               // push eax (restore return address into stack)
			0x8b, 0x44, 0x24, 0xfc,             // mov eax, DWORD PTR [esp-4] (restore eax from backup)
			0xff, 0x15, 0xff, 0xff, 0xff, 0xff, // call 0xffffffff

			// backup eax on the stack and get the original return address into eax
			0x50, // push eax
			0x83, 0xc4, 0x04, // add esp, 0x04
			0x58, // pop eax

			// move return address to it's original position and restore eax
			0x83, 0xc4, 0x08, // add esp, 8
			0x50, // push eax
			0x8b, 0x44, 0x24, 0xf4, // mov eax, DWORD PTR[esp-12]

			0xc3, // ret
		};

		m_Size = sizeof(opCodes);

		m_Blob = (BYTE*)VirtualAlloc(NULL, m_Size, MEM_COMMIT, PAGE_READWRITE);
		if (m_Blob)
		{
			memcpy(m_Blob, opCodes, m_Size);
			*((void**)&m_Blob[6]) = memberFnPtr;
			*((void**)&m_Blob[11]) = &obj;
			*((void**)&m_Blob[22]) = &redirectorFnPtr;

			DWORD oldProtect = 0;
			if (!VirtualProtect(m_Blob, m_Size, PAGE_EXECUTE, &oldProtect))
			{
				VirtualFree(m_Blob, m_Size, MEM_RELEASE);
				m_Blob = nullptr;
			}
		}
	}

	~TrampolineToMethod()
	{
		if (m_Blob)
		{
			VirtualFree(m_Blob, m_Size, MEM_RELEASE);
		}
	}

	TFunc GetFuncPtr() const
	{
		return reinterpret_cast<TFunc>(m_Blob);
	}

private:
	template<typename TClass, typename TMemberFunc, std::size_t N, typename Indices = std::make_index_sequence<N>>
	void* GenerateFuncPtr(TClass* obj)
	{
		return GenerateFuncPtr_Impl<TClass, TMemberFunc>(obj, Indices{});
	}

	template<typename TClass, typename TMemberFunc, std::size_t... I>
	void* GenerateFuncPtr_Impl(TClass* obj, std::index_sequence<I...>)
	{
		return (void*)&Redirector<TClass, TMemberFunc, typename std::tuple_element<I, typename function_traits<TFunc>::type_list>::type...>;
	}

	template<typename TClass, typename TMemberFunc, typename... Args>
	static typename function_traits<TFunc>::result_type __cdecl Redirector(void* savedReturnPointer, TClass* obj, TMemberFunc fn, Args ... args)
	{
		return (obj->*fn)(args...);
	}

	BYTE* m_Blob;
	size_t m_Size;
};
#else
#error TrampolineToMethod not supported in 64 bit
#endif
