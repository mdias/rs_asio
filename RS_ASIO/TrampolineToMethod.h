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
			0x59, // pop ecx (save return address)
			0xb8, 0xff, 0xff, 0xff, 0xff, // mov eax, 0xffffffff (store fn pointer)
			0x50, // push eax
			0xb8, 0xff, 0xff, 0xff, 0xff, // mov eax, 0xffffffff (store this pointer)
			0x50, // push eax
			0x51, // push ecx (restore return address)
			0xff, 0x15, 0xff, 0xff, 0xff, 0xff, // call 0xffffffff

			// note that we avoid touching eax as that contains the result value
			0x59, // pop ecx
			0x5b, // pop ebx
			0x5b, // pop ebx
			0x51, // push ecx
			0xc3, // ret
		};

		m_Size = sizeof(opCodes);

		m_Blob = (BYTE*)VirtualAlloc(NULL, m_Size, MEM_COMMIT, PAGE_READWRITE);
		if (m_Blob)
		{
			memcpy(m_Blob, opCodes, m_Size);
			*((void**)&m_Blob[2]) = memberFnPtr;
			*((void**)&m_Blob[8]) = &obj;
			*((void**)&m_Blob[16]) = &redirectorFnPtr;

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
		return (void*)&Redirector<TClass, TMemberFunc, std::tuple_element<I, function_traits<TFunc>::type_list>::type...>;
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