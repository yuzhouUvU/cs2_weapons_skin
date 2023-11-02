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
public:
	SCHEMA_FIELD(uint16_t, CEconItemAttribute, m_iAttributeDefinitionIndex);
	SCHEMA_FIELD(float, CEconItemAttribute, m_flValue);
	SCHEMA_FIELD(float, CEconItemAttribute, m_flInitialValue);
};

class CAttributeList
{
public:
	SCHEMA_FIELD(int64_t, CAttributeList, m_Attributes);
};

class CEconItemView
{
public:
	SCHEMA_FIELD(int32_t, CEconItemView, m_iEntityQuality);
	SCHEMA_FIELD(CAttributeList, CEconItemView, m_AttributeList);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iItemIDHigh);
	SCHEMA_FIELD(int32_t, CEconItemView, m_iAccountID);
	SCHEMA_FIELD(uint16_t, CEconItemView, m_iItemDefinitionIndex);
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