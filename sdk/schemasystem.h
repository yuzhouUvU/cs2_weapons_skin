//====== Copyright �, Valve Corporation, All rights reserved. =======
//
// Purpose: Additional shared object cache functionality for the GC
//
//=============================================================================

#ifndef SCHEMASYSTEM_H
#define SCHEMASYSTEM_H
#ifdef _WIN32
#pragma once
#endif

#include <cstdint>
#include <type_traits>

#if defined(_WIN32)
#define FASTCALL __fastcall
#define THISCALL __thiscall
#else
#define FASTCALL __attribute__((fastcall))
#define THISCALL
#define strtok_s strtok_r
#endif

#define CALL_VIRTUAL(retType, idx, ...) \
    vmt::CallVirtual<retType>(idx, __VA_ARGS__)
namespace vmt {
	template <typename T = void*>
	inline T GetVMethod(uint32_t uIndex, void* pClass) {
		void** pVTable = *static_cast<void***>(pClass);
		return reinterpret_cast<T>(pVTable[uIndex]);
	}

	template <typename T, typename... Args>
	inline T CallVirtual(uint32_t uIndex, void* pClass, Args... args) {
		auto pFunc = GetVMethod<T(THISCALL*)(void*, Args...)>(uIndex, pClass);
		return pFunc(pClass, args...);
	}
}  // namespace vmt

struct SchemaClassFieldData_t
{
    const char* m_pszName; // 0x0000
    void* m_pSchemaType; // 0x0008
    int32_t m_iOffset; // 0x0010
    int32_t m_iMetaDataSize; // 0x0014
    void* m_pMetaData; // 0x0018
};

struct SchemaClassInfoData_t
{
    char pad_0x0000[0x8]; // 0x0000

    const char* m_pszName; // 0x0008
    const char* m_pszModule; // 0x0010

    int m_iSize; // 0x0018
    int16_t m_iFieldsCount; // 0x001C

    int16_t m_iStaticSize; // 0x001E
    int16_t m_iMetadataSize; // 0x0020
    int16_t m_iUnk1; // 0x0022
    int16_t m_iUnk2; // 0x0024
    int16_t m_iUnk3; // 0x0026

    SchemaClassFieldData_t* m_pFieldsData; // 0x0028
};

class CSchemaSystemTypeScope
{
public:
    void FindDeclaredClass(SchemaClassInfoData_t*& pClassInfo, const char* pszClassName);
};

class CSchemaSystem
{
public:
    CSchemaSystemTypeScope* FindTypeScopeForModule(const char* szpModuleName);
    CSchemaSystemTypeScope* GetServerTypeScope();
    int32_t GetServerOffset(const char* pszClassName, const char* pszPropName);
};

extern CSchemaSystem* g_pCSchemaSystem;

#define SCHEMA_FIELD(type, className, propName)                                                        \
    std::add_lvalue_reference_t<type> propName()                                                       \
    {                                                                                                  \
        static const int32_t offset = g_pCSchemaSystem->GetServerOffset(#className, #propName);         \
        return *reinterpret_cast<std::add_pointer_t<type>>(reinterpret_cast<intptr_t>(this) + offset); \
    }


#endif //SCHEMASYSTEM_H
