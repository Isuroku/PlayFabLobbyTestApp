#pragma once

#include "CoreMinimal.h"
#include "Misc/CString.h"


static inline uint64_t NextPowerOfTwo(uint64_t x)
{
	x -= 1;
	x |= (x >> 1);
	x |= (x >> 2);
	x |= (x >> 4);
	x |= (x >> 8);
	x |= (x >> 16);

	return x + 1;
}

template<int32 N>
class FStringBuilder
{
public:
	FStringBuilder()
	{
		Base = StackBuffer;
		CurPos = Base;
		End = Base + N;
		if(N > 0)
			*CurPos = (TCHAR)0;
	}

	~FStringBuilder()
	{
		if (bIsDynamic)
			FMemory::Free(Base);
	}

	FStringBuilder(const FStringBuilder&) = delete;
	FStringBuilder(FStringBuilder&&) = delete;

	const FStringBuilder& operator=(const FStringBuilder&) = delete;
	const FStringBuilder& operator=(FStringBuilder&&) = delete;

	inline uint64	Len() const { return int32(CurPos - Base); }
	inline const TCHAR* ToString() const { return Base; }
	inline const TCHAR* operator*() const { return Base; }

	inline const TCHAR	LastChar() const { return *(CurPos - 1); }

	inline FStringBuilder& Append(TCHAR Char)
	{
		EnsureCapacity(2);

		*CurPos++ = Char;
		*CurPos = (TCHAR)0;
		
		return *this;
	}

	inline FStringBuilder& Append(const TCHAR* NulTerminatedString)
	{
		const int32 Length = TCString<TCHAR>::Strlen(NulTerminatedString);
		return Append(NulTerminatedString, Length);
	}

	inline FStringBuilder& Append(const FString& InString)
	{
		return Append(*InString, InString.Len());
	}

	FStringBuilder& AppendFmt(const TCHAR* Fmt, ...)
	{
		va_list args;
		va_start(args, Fmt);

		uint64 FreeSize = End - CurPos - 1;
		// First try to print to a stack allocated location
		int32 Result = FCString::GetVarArgs(CurPos, FreeSize, Fmt, args);

		// If that fails, start allocating regular memory
		if (Result == -1)
		{
			while (Result == -1)
			{
				Extend(N);
				FreeSize = End - CurPos - 1;
				Result = FCString::GetVarArgs(CurPos, FreeSize, Fmt, args);
			};
		}

		CurPos += Result;
		*CurPos = (TCHAR)0;

		va_end(args);

		return *this;
	}

protected:

	inline FStringBuilder& Append(const TCHAR* NulTerminatedString, int32 Length)
	{
		EnsureCapacity(Length + 1);
		TCHAR* RESTRICT Dest = CurPos;
		CurPos += Length;

		FMemory::Memcpy(Dest, NulTerminatedString, Length * sizeof(TCHAR));

		*CurPos = 0;

		return *this;
	}

	inline void EnsureCapacity(int32 RequiredAdditionalCapacity)
	{
		// precondition: we know the current buffer has enough capacity
		// for the existing string including NUL terminator

		if ((CurPos + RequiredAdditionalCapacity) < End)
			return;

		Extend(RequiredAdditionalCapacity);
	}

	void Extend(int32 ExtraCapacity)
	{
		const uint64 OldCapacity = End - Base;
		const uint64 NewCapacity = NextPowerOfTwo(OldCapacity + ExtraCapacity);
		uint64 Pos = CurPos - Base;

		TCHAR* NewBase = (TCHAR*)FMemory::Malloc(NewCapacity * sizeof(TCHAR));

		FMemory::Memcpy(NewBase, Base, Pos * sizeof(TCHAR));

		if (bIsDynamic)
		{
			FMemory::Free(Base);
		}

		Base = NewBase;
		CurPos = NewBase + Pos;
		End = NewBase + NewCapacity;
		bIsDynamic = true;
	}

	TCHAR	StackBuffer[N];
	TCHAR*	Base;
	TCHAR*	CurPos;
	TCHAR*	End;
	bool	bIsDynamic = false;
};

typedef FStringBuilder<512> FStringBuilder512;