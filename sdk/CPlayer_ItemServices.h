#pragma once
#include "CPlayerPawnComponent.h"
#include "schemasystem.h"
#include "ehandle.h"
#include "CBaseFlex.h"

class CEconEntity : public CBaseFlex
{
public:
};

class CEconItemAttribute
{
private:
	[[maybe_unused]] uint8_t __pad0000[0x30]; // 0x0
public:
	// MNetworkEnable
	uint16_t m_iAttributeDefinitionIndex; // 0x30	
private:
	[[maybe_unused]] uint8_t __pad0032[0x2]; // 0x32
public:
	// MNetworkEnable
	// MNetworkAlias "m_iRawValue32"
	int m_flValue; // 0x34	
	// MNetworkEnable
	int m_flInitialValue; // 0x38	
	// MNetworkEnable
	int32_t m_nRefundableCurrency; // 0x3c	
	// MNetworkEnable
	bool m_bSetBonus; // 0x40	

	inline CEconItemAttribute(uint16_t iAttributeDefinitionIndex, int flValue)
	{
		m_iAttributeDefinitionIndex = iAttributeDefinitionIndex;
		m_flValue = flValue;
	}
};

class CAttributeList
{
private:
	[[maybe_unused]] uint8_t __pad0000[0x8]; // 0x0
public:
	// MNetworkEnable
	// MNetworkTypeAlias "CUtlVector< CEconItemAttribute >"
	CUtlVector<CEconItemAttribute, CUtlMemory<CEconItemAttribute> > m_Attributes;
	void* m_pManager; // 0x58

	inline void AddAttribute(int iIndex, int flValue)
	{
		m_Attributes.AddToTail(CEconItemAttribute(iIndex, flValue));
	}
};

class CEconItemView
{
public:
	SCHEMA_FIELD(int32_t, CEconItemView, m_iEntityQuality);
	SCHEMA_FIELD(CAttributeList, CEconItemView, m_AttributeList);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemIDHigh);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemIDLow);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iAccountID);
	SCHEMA_FIELD(uint16_t, CEconItemView, m_iItemDefinitionIndex);
	SCHEMA_FIELD(bool, CEconItemView, m_bInitialized);
	SCHEMA_FIELD(uint64_t, CEconItemView, m_iItemID);
};

class CAttributeContainer
{
public:
	SCHEMA_FIELD(CEconItemView, CAttributeContainer, m_Item);
};

class CModelState
{
public:
	SCHEMA_FIELD(uint64_t, CModelState, m_MeshGroupMask);
};

class CSkeletonInstance
{
public:
	SCHEMA_FIELD(CModelState, CSkeletonInstance, m_modelState);
};

class CGameSceneNode
{
public:
	CSkeletonInstance* GetSkeletonInstance() {
        return CALL_VIRTUAL(CSkeletonInstance*, 8, this);
    }
};

class CBodyComponent
{
public:
	SCHEMA_FIELD(CGameSceneNode*, CBodyComponent, m_pSceneNode);
};

class CBasePlayerWeapon : public CEconEntity
{
public:
	SCHEMA_FIELD(CBodyComponent*, CBaseEntity, m_CBodyComponent);
	SCHEMA_FIELD(CAttributeContainer, CEconEntity, m_AttributeManager);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackPaintKit);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackSeed);
	SCHEMA_FIELD(int32_t, CEconEntity, m_nFallbackStatTrak);
	SCHEMA_FIELD(float, CEconEntity, m_flFallbackWear);
	SCHEMA_FIELD(uint64_t, CEconEntity, m_OriginalOwnerXuidLow);
};

class CPlayer_WeaponServices : public CPlayerPawnComponent
{
public:
	virtual ~CPlayer_WeaponServices() = 0;
	SCHEMA_FIELD(CHandle<CBasePlayerWeapon>, CPlayer_WeaponServices, m_hActiveWeapon);
	auto RemoveWeapon(CBasePlayerWeapon* weapon) {
        return CALL_VIRTUAL(void, 20, this, weapon, nullptr, nullptr);
    }
};

class CPlayer_ItemServices : public CPlayerPawnComponent
{
public:
	virtual ~CPlayer_ItemServices() = 0;
};


class CEconWearable : public CEconEntity
{
public:
	SCHEMA_FIELD(int32_t, CEconWearable, m_nForceSkin);
	SCHEMA_FIELD(bool, CEconWearable, m_bAlwaysAllow);
};