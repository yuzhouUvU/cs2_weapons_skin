#pragma once
#include "CCSPlayerPawnBase.h"
#include "schemasystem.h"

class CCSPlayerPawn : public CCSPlayerPawnBase
{
public:
	SCHEMA_FIELD(CEconItemView, CCSPlayerPawn, m_EconGloves);
};