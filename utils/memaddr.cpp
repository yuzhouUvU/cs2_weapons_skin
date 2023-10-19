#include "memaddr.h"

//-----------------------------------------------------------------------------
// Purpose: ResolveRelativeAddress wrapper
// Input  : opcodeOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::FollowNearCall(const ptrdiff_t opcodeOffset, const ptrdiff_t nextInstructionOffset)
{
	return ResolveRelativeAddress(opcodeOffset, nextInstructionOffset);
}

//-----------------------------------------------------------------------------
// Purpose: ResolveRelativeAddressSelf wrapper
// Input  : opcodeOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::FollowNearCallSelf(const ptrdiff_t opcodeOffset, const ptrdiff_t nextInstructionOffset)
{
	return ResolveRelativeAddressSelf(opcodeOffset, nextInstructionOffset);
}

//-----------------------------------------------------------------------------
// Purpose: resolves the relative pointer to offset
// Input  : registerOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::ResolveRelativeAddress(const ptrdiff_t registerOffset, const ptrdiff_t nextInstructionOffset)
{
	// Skip register.
	const uintptr_t skipRegister = ptr + registerOffset;

	// Get 4-byte long relative Address.
	const int32_t relativeAddress = *reinterpret_cast<int32_t*>(skipRegister);

	// Get location of next instruction.
	const uintptr_t nextInstruction = ptr + nextInstructionOffset;

	// Get function location via adding relative Address to next instruction.
	return CMemory(nextInstruction + relativeAddress);
}

//-----------------------------------------------------------------------------
// Purpose: resolves the relative pointer to offset from current address
// Input  : registerOffset - 
//			nextInstructionOffset - 
// Output : CMemory
//-----------------------------------------------------------------------------
CMemory CMemory::ResolveRelativeAddressSelf(const ptrdiff_t registerOffset, const ptrdiff_t nextInstructionOffset)
{
	// Skip register.
	const uintptr_t skipRegister = ptr + registerOffset;

	// Get 4-byte long relative Address.
	const int32_t relativeAddress = *reinterpret_cast<int32_t*>(skipRegister);

	// Get location of next instruction.
	const uintptr_t nextInstruction = ptr + nextInstructionOffset;

	// Get function location via adding relative Address to next instruction.
	ptr = nextInstruction + relativeAddress;
	return *this;
}
