#include <stdio.h>
#include "Skin.h"
#include "metamod_oslink.h"
#include "utils.hpp"
#include <utlstring.h>
#include <KeyValues.h>
#include "sdk/schemasystem.h"
#include "sdk/CBaseEntity.h"
#include "sdk/CGameRulesProxy.h"
#include "sdk/CBasePlayerPawn.h"
#include "sdk/CCSPlayerController.h"
#include "sdk/CCSPlayer_ItemServices.h"
#include "sdk/CSmokeGrenadeProjectile.h"
#include <map>
#ifdef _WIN32
#include <Windows.h>
#include <TlHelp32.h>
#else
#include "utils/module.h"
#endif
#include <string>

Skin g_Skin;
PLUGIN_EXPOSE(Skin, g_Skin);
IVEngineServer2* engine = nullptr;
IGameEventManager2* gameeventmanager = nullptr;
IGameResourceServiceServer* g_pGameResourceService = nullptr;
CGameEntitySystem* g_pGameEntitySystem = nullptr;
CEntitySystem* g_pEntitySystem = nullptr;
CSchemaSystem* g_pCSchemaSystem = nullptr;
CCSGameRules* g_pGameRules = nullptr;
CPlayerSpawnEvent g_PlayerSpawnEvent;
CRoundPreStartEvent g_RoundPreStartEvent;
CEntityListener g_EntityListener;
bool g_bPistolRound;

typedef struct SkinParm
{
	int m_nFallbackPaintKit;
	int m_nFallbackSeed;
	float m_flFallbackWear;
}SkinParm;

typedef struct Sticker
{
	int sticker_id;
	int sticker_pos;
}Sticker;

#ifdef _WIN32
typedef void*(FASTCALL* SubClassChange_t)(const CCommandContext &context, const CCommand &args);
typedef void*(FASTCALL* EntityRemove_t)(CGameEntitySystem*, void*, void*,uint64_t);
typedef void(FASTCALL* GiveNamedItem_t)(void* itemService,const char* pchName, void* iSubType,void* pScriptItem, void* a5,void* a6);
typedef void(FASTCALL* UTIL_ClientPrintAll_t)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4);
typedef void(FASTCALL* UTIL_ClientPrint_t)(CBasePlayerController *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4);

extern SubClassChange_t FnSubClassChange;
extern EntityRemove_t FnEntityRemove;
extern GiveNamedItem_t FnGiveNamedItem;
extern UTIL_ClientPrintAll_t FnUTIL_ClientPrintAll;
extern UTIL_ClientPrint_t FnUTIL_ClientPrint;
EntityRemove_t FnEntityRemove;
GiveNamedItem_t FnGiveNamedItem;
UTIL_ClientPrintAll_t FnUTIL_ClientPrintAll;
UTIL_ClientPrint_t FnUTIL_ClientPrint;
SubClassChange_t FnSubClassChange;
#else
void (*FnSubClassChange)(const CCommandContext &context, const CCommand &args) = nullptr;
void (*FnEntityRemove)(CGameEntitySystem*, void*, void*,uint64_t) = nullptr;
void (*FnGiveNamedItem)(void* itemService,const char* pchName, void* iSubType,void* pScriptItem, void* a5,void* a6) = nullptr;
void (*FnUTIL_ClientPrintAll)(int msg_dest, const char* msg_name, const char* param1, const char* param2, const char* param3, const char* param4) = nullptr;
void (*FnUTIL_ClientPrint)(CBasePlayerController *player, int msg_dest, const char *msg_name, const char *param1, const char *param2, const char *param3, const char *param4) = nullptr;
#endif

std::map<int, std::string> g_WeaponsMap;
std::map<uint64_t, int> g_PlayerKnifes;
std::map<uint64_t, std::map<int, SkinParm>> g_PlayerSkins;
std::map<uint64_t, std::map<int, Sticker>> g_Sticker;

class GameSessionConfiguration_t { };
SH_DECL_HOOK3_void(INetworkServerService, StartupServer, SH_NOATTRIB, 0, const GameSessionConfiguration_t&, ISource2WorldSession*, const char*);
SH_DECL_HOOK3_void(IServerGameDLL, GameFrame, SH_NOATTRIB, 0, bool, bool, bool);

#ifdef _WIN32
inline void* FindSignature(const char* modname,const char* sig)
{
	DWORD64 hModule = (DWORD64)GetModuleHandle(modname);
	if (!hModule)
	{
		return NULL;
	}
	HANDLE hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	MODULEENTRY32 mod = {sizeof(MODULEENTRY32)};
	
	while (Module32Next(hSnap, &mod))
	{
		if (!strcmp(modname, mod.szModule))
		{
			if(!strstr(mod.szExePath,"metamod"))
				break;
		}
	}
	CloseHandle(hSnap);
	byte* b_sig = (byte*)sig;
	int sig_len = strlen(sig);
	byte* addr = (byte*)mod.modBaseAddr;
	for (int i = 0; i < mod.modBaseSize; i++)
	{
		int flag = 0;
		for (int n = 0; n < sig_len; n++)
		{
			if (i + n >= mod.modBaseSize)break;
			if (*(b_sig + n)=='\x3F' || *(b_sig + n) == *(addr + i+ n))
			{
				flag++;
			}
		}
		if (flag == sig_len)
		{
			return (void*)(addr + i);
		}
	}
	return NULL;
}
#endif

bool Skin::Load(PluginId id, ISmmAPI* ismm, char* error, size_t maxlen, bool late)
{
	PLUGIN_SAVEVARS();

	GET_V_IFACE_CURRENT(GetEngineFactory, engine, IVEngineServer2, SOURCE2ENGINETOSERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pCVar, ICvar, CVAR_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetServerFactory, g_pSource2Server, ISource2Server, SOURCE2SERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pNetworkServerService, INetworkServerService, NETWORKSERVERSERVICE_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetEngineFactory, g_pGameResourceService, IGameResourceServiceServer, GAMERESOURCESERVICESERVER_INTERFACE_VERSION);
	GET_V_IFACE_CURRENT(GetFileSystemFactory, g_pFullFileSystem, IFileSystem, FILESYSTEM_INTERFACE_VERSION);

	// Get CSchemaSystem
	{
		HINSTANCE m_hModule = dlmount(WIN_LINUX("schemasystem.dll", "libschemasystem.so"));
		g_pCSchemaSystem = reinterpret_cast<CSchemaSystem*>(reinterpret_cast<CreateInterfaceFn>(dlsym(m_hModule, "CreateInterface"))(SCHEMASYSTEM_INTERFACE_VERSION, nullptr));
		dlclose(m_hModule);
	}

	SH_ADD_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Skin::StartupServer), true);
	SH_ADD_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Skin::GameFrame), true);

	gameeventmanager = static_cast<IGameEventManager2*>(CallVFunc<IToolGameEventAPI*, 91>(g_pSource2Server));

	ConVar_Register(FCVAR_GAMEDLL);

	g_WeaponsMap = {{59,"weapon_knife"},{42,"weapon_knife"},{26,"weapon_bizon"},{27,"weapon_mac10"},{34,"weapon_mp9"},{19,"weapon_p90"},{24,"weapon_ump45"},{7,"weapon_ak47"},{8,"weapon_aug"},{10,"weapon_famas"},{13,"weapon_galilar"},{16,"weapon_m4a1"},{60,"weapon_m4a1_silencer"},{39,"weapon_sg556"},{9,"weapon_awp"},{11,"weapon_g3sg1"},{38,"weapon_scar20"},{40,"weapon_ssg08"},{29,"weapon_mag7"},{35,"weapon_nova"},{29,"weapon_sawedoff"},{25,"weapon_xm1014"},{14,"weapon_m249"},{9,"weapon_awp"},{28,"weapon_negev"},{1,"weapon_deagle"},{2,"weapon_elite"},{3,"weapon_fiveseven"},{4,"weapon_glock"},{32,"weapon_hkp2000"},{36,"weapon_p250"},{30,"weapon_tec9"},{61,"weapon_usp_silencer"},{63,"weapon_cz75a"},{64,"weapon_revolver"}};
		
	#ifdef _WIN32	
	byte* vscript = (byte*)FindSignature("vscript.dll", "\xBE\x01\x3F\x3F\x3F\x2B\xD6\x74\x61\x3B\xD6");
	if(vscript)
	{
		DWORD pOld;
		VirtualProtect(vscript, 2, PAGE_EXECUTE_READWRITE, &pOld);
		*(vscript + 1) = 2;
		VirtualProtect(vscript, 2, pOld, &pOld);
	}
	#endif
	return true;
}

bool Skin::Unload(char *error, size_t maxlen)
{
	SH_REMOVE_HOOK(IServerGameDLL, GameFrame, g_pSource2Server, SH_MEMBER(this, &Skin::GameFrame), true);
	SH_REMOVE_HOOK(INetworkServerService, StartupServer, g_pNetworkServerService, SH_MEMBER(this, &Skin::StartupServer), true);

	gameeventmanager->RemoveListener(&g_PlayerSpawnEvent);
	gameeventmanager->RemoveListener(&g_RoundPreStartEvent);

	g_pGameEntitySystem->RemoveListenerEntity(&g_EntityListener);

	ConVar_Unregister();
	
	return true;
}

void Skin::NextFrame(std::function<void()> fn)
{
	m_nextFrame.push_back(fn);
}

void Skin::StartupServer(const GameSessionConfiguration_t& config, ISource2WorldSession*, const char*)
{
	#ifdef _WIN32
	FnUTIL_ClientPrint = (UTIL_ClientPrint_t)FindSignature("server.dll", "\x48\x85\xC9\x0F\x84\x3F\x3F\x3F\x3F\x48\x8B\xC4\x48\x89\x58\x18");
	FnUTIL_ClientPrintAll = (UTIL_ClientPrintAll_t)FindSignature("server.dll", "\x48\x89\x5C\x24\x08\x48\x89\x6C\x24\x10\x48\x89\x74\x24\x18\x57\x48\x81\xEC\x70\x01\x3F\x3F\x8B\xE9");
	FnGiveNamedItem = (GiveNamedItem_t)FindSignature("server.dll", "\x48\x89\x5C\x24\x18\x48\x89\x74\x24\x20\x55\x57\x41\x54\x41\x56\x41\x57\x48\x8D\x6C\x24\xD9");
	FnEntityRemove = (EntityRemove_t)FindSignature("server.dll", "\x48\x85\xD2\x0F\x3F\x3F\x3F\x3F\x3F\x57\x48\x3F\x3F\x3F\x48\x89\x3F\x3F\x3F\x48\x8B\xF9\x48\x8B");
	FnSubClassChange = (SubClassChange_t)FindSignature("server.dll", "\x40\x55\x41\x57\x48\x83\xEC\x78\x83\xBA\x38\x04");
	#else
	CModule libserver(g_pSource2Server);
	FnUTIL_ClientPrint = libserver.FindPatternSIMD("55 48 89 E5 41 57 49 89 CF 41 56 49 89 D6 41 55 41 89 F5 41 54 4C 8D A5 A0 FE FF FF").RCast< decltype(FnUTIL_ClientPrint) >();
	FnUTIL_ClientPrintAll = libserver.FindPatternSIMD("55 48 89 E5 41 57 49 89 D7 41 56 49 89 F6 41 55 41 89 FD").RCast< decltype(FnUTIL_ClientPrintAll) >();
	FnGiveNamedItem = libserver.FindPatternSIMD("55 48 89 E5 41 57 41 56 49 89 CE 41 55 49 89 F5 41 54 49 89 D4 53 48 89").RCast<decltype(FnGiveNamedItem)>();
	FnEntityRemove = libserver.FindPatternSIMD("48 85 F6 74 0B 48 8B 76 10 E9 B2 FE FF FF").RCast<decltype(FnEntityRemove)>();
	FnSubClassChange = libserver.FindPatternSIMD("55 48 89 E5 41 57 41 56 41 55 41 54 53 48 81 EC C8 00 00 00 83 BE 38 04 00 00 01 0F 8E 47 02").RCast<decltype(FnSubClassChange)>();
	#endif
	g_pGameRules = nullptr;

	static bool bDone = false;
	if (!bDone)
	{
		g_pGameEntitySystem = *reinterpret_cast<CGameEntitySystem**>(reinterpret_cast<uintptr_t>(g_pGameResourceService) + WIN_LINUX(0x58, 0x50));
		g_pEntitySystem = g_pGameEntitySystem;

		g_pGameEntitySystem->AddListenerEntity(&g_EntityListener);

		gameeventmanager->AddListener(&g_PlayerSpawnEvent, "player_spawn", true);
		gameeventmanager->AddListener(&g_RoundPreStartEvent, "round_prestart", true);

		bDone = true;
	}
}

void Skin::GameFrame(bool simulating, bool bFirstTick, bool bLastTick)
{
	if (!g_pGameRules)
	{
		CCSGameRulesProxy* pGameRulesProxy = static_cast<CCSGameRulesProxy*>(UTIL_FindEntityByClassname(nullptr, "cs_gamerules"));
		if (pGameRulesProxy)
		{
			g_pGameRules = pGameRulesProxy->m_pGameRules();
		}
	}
	
	while (!m_nextFrame.empty())
	{
		m_nextFrame.front()();
		m_nextFrame.pop_front();
	}
}

void CPlayerSpawnEvent::FireGameEvent(IGameEvent* event)
{
	if (!g_pGameRules || g_pGameRules->m_bWarmupPeriod())
		return;
	CBasePlayerController* pPlayerController = static_cast<CBasePlayerController*>(event->GetPlayerController("userid"));
	if (!pPlayerController || pPlayerController->m_steamID() == 0) // Ignore bots
		return;
	// g_Skin.NextFrame([hPlayerController = CHandle<CBasePlayerController>(pPlayerController)]()
	// {
	// 	CCSPlayerController* pPlayerController = static_cast<CCSPlayerController*>(hPlayerController.Get());
	// 	if (!pPlayerController)
	// 		return;

	// 	CCSPlayerPawn* pPlayerPawn = pPlayerController->m_hPlayerPawn();
	// 	if (!pPlayerPawn || pPlayerPawn->m_lifeState() != LIFE_ALIVE)
	// 		return;
	// });
}

void CRoundPreStartEvent::FireGameEvent(IGameEvent* event)
{
	if (g_pGameRules)
	{
		g_bPistolRound = g_pGameRules->m_totalRoundsPlayed() == 0 || (g_pGameRules->m_bSwitchingTeamsAtRoundReset() && g_pGameRules->m_nOvertimePlaying() == 0) || g_pGameRules->m_bGameRestart();
	}
}

void CEntityListener::OnEntitySpawned(CEntityInstance* pEntity)
{
	#ifdef _WIN32
	try
	{
	#endif
		CBasePlayerWeapon* pBasePlayerWeapon = dynamic_cast<CBasePlayerWeapon*>(pEntity);
		if(!pBasePlayerWeapon)return;	

		g_Skin.NextFrame([pBasePlayerWeapon = pBasePlayerWeapon]()
		{
			int64_t steamid = pBasePlayerWeapon->m_OriginalOwnerXuidLow();
			if(!steamid)return;
			int64_t weaponId = pBasePlayerWeapon->m_AttributeManager().m_Item().m_iItemDefinitionIndex();

			auto weapon = g_PlayerSkins.find(steamid);
			if(weapon == g_PlayerSkins.end())return;
			auto skin_parm = weapon->second.find(weaponId);
			if(skin_parm == weapon->second.end())return;
			
			pBasePlayerWeapon->m_nFallbackPaintKit() = skin_parm->second.m_nFallbackPaintKit;
			pBasePlayerWeapon->m_nFallbackSeed() = skin_parm->second.m_nFallbackSeed;
			pBasePlayerWeapon->m_flFallbackWear() = skin_parm->second.m_flFallbackWear;

			pBasePlayerWeapon->m_AttributeManager().m_Item().m_iItemIDHigh() = -1;

			auto weapon_sticker = g_Sticker.find(steamid);
			if(weapon_sticker != g_Sticker.end())
			{
				auto sticker_parm = weapon_sticker->second.find(weaponId);
				if(sticker_parm != weapon_sticker->second.end())
					pBasePlayerWeapon->m_AttributeManager().m_Item().m_AttributeList().AddAttribute(sticker_parm->second.sticker_pos,sticker_parm->second.sticker_id); //sticker slot 0 id
			}

			if(weaponId == 59 || weaponId == 42)
			{
				auto knife_idx = g_PlayerKnifes.find(steamid);
				if(knife_idx == g_PlayerKnifes.end())return;
				
				char buf[64] = {0};
				int index = static_cast<CEntityInstance*>(pBasePlayerWeapon)->m_pEntity->m_EHandle.GetEntryIndex();
				sprintf(buf,"i_subclass_change %d %d",knife_idx->second,index);
				engine->ServerCommand(buf);
			}
			else
			{
				if(!pBasePlayerWeapon->m_AttributeManager().m_Item().m_iAccountID() && pBasePlayerWeapon->m_CBodyComponent() && pBasePlayerWeapon->m_CBodyComponent()->m_pSceneNode())
				{
					pBasePlayerWeapon->m_CBodyComponent()->m_pSceneNode()->GetSkeletonInstance()->m_modelState().m_MeshGroupMask() = 2;
				}
			}
			//META_CONPRINTF( "class: %s\n", static_cast<CEntityInstance*>(pBasePlayerWeapon)->m_pEntity->m_designerName.String());
			META_CONPRINTF( "steamId: %lld itemId: %d\n", steamid, weaponId);
		});
	#ifdef _WIN32
	}
	catch(...){}
	#endif
}

CON_COMMAND_F(skin, "修改皮肤", FCVAR_CLIENT_CAN_EXECUTE)
{
	if(context.GetPlayerSlot() == -1)return;
	CCSPlayerController* pPlayerController = (CCSPlayerController*)g_pEntitySystem->GetBaseEntity((CEntityIndex)(context.GetPlayerSlot().Get() + 1));
	CCSPlayerPawn* pPlayerPawn = pPlayerController->m_hPlayerPawn();
	if (!pPlayerPawn || pPlayerPawn->m_lifeState() != LIFE_ALIVE)
		return;
	
	CPlayer_WeaponServices* pWeaponServices = pPlayerPawn->m_pWeaponServices();

	int64_t steamid = pPlayerController->m_steamID();
	int64_t weaponId = pWeaponServices->m_hActiveWeapon()->m_AttributeManager().m_Item().m_iItemDefinitionIndex();
	
	auto weapon_name = g_WeaponsMap.find(weaponId);
	if(weapon_name == g_WeaponsMap.end())return;
	
	if(args.ArgC() == 1)
	{
		FnUTIL_ClientPrint(pPlayerController, 3, " \x04 [SKIN] \x01访问：http://skin.ymos.top/ 生成皮肤修改参数",nullptr, nullptr, nullptr, nullptr);
		FnUTIL_ClientPrint(pPlayerController, 3, " \x04 [SKIN] \x01开源仓库：https://github.com/yuzhouUvU/cs2_weapons_skin",nullptr, nullptr, nullptr, nullptr);
		return;
	}
	char buf[255] = {0};
	if(weaponId == 59 || weaponId == 42)
	{
		if(args.ArgC() != 5)
		{
			FnUTIL_ClientPrint(pPlayerController, 3, " \x04 [SKIN] \x01修改刀具控制台输入 'skin 编号 模板 磨损 刀具编号'",nullptr, nullptr, nullptr, nullptr);
			return;
		}
		g_PlayerKnifes[steamid] = atoi(args.Arg(4));
	}
	else
	{
		if(args.ArgC() != 4 && args.ArgC() != 6)
		{
			FnUTIL_ClientPrint(pPlayerController, 3, " \x04 [SKIN] \x01修改武器皮肤控制台输入 'skin 编号 模板 磨损'",nullptr, nullptr, nullptr, nullptr);
			FnUTIL_ClientPrint(pPlayerController, 3, " \x04 [SKIN] \x01添加武器贴纸控制台输入 'skin 编号 模板 磨损 贴纸编号 位置(0-5)'",nullptr, nullptr, nullptr, nullptr);
			return;
		}
	}

	if(args.ArgC() == 6 && weaponId!= 59 && weaponId!= 42)
	{
		g_Sticker[steamid][weaponId].sticker_id = atoi(args.Arg(4));
		int pos = atoi(args.Arg(5));
		if(pos > 5 || pos < 0)
		{
			FnUTIL_ClientPrint(pPlayerController, 3, " \x04 [SKIN] \x01位置请输入(0-5)之间的数字 ",nullptr, nullptr, nullptr, nullptr);
			return;
		}
		switch (pos)
		{
		case 0:
			g_Sticker[steamid][weaponId].sticker_pos = 113;
			break;
		case 1:
			g_Sticker[steamid][weaponId].sticker_pos = 117;
			break;
		case 2:
			g_Sticker[steamid][weaponId].sticker_pos = 121;
			break;
		case 3:
			g_Sticker[steamid][weaponId].sticker_pos = 125;
			break;
		case 4:
			g_Sticker[steamid][weaponId].sticker_pos = 129;
			break;
		case 5:
			g_Sticker[steamid][weaponId].sticker_pos = 133;
			break;
		default:
			break;
		}

		sprintf(buf, " \x04 [SKIN] \x01已修改贴纸 贴纸编号:%d 位置:%d",g_Sticker[steamid][weaponId].sticker_id, pos);
		FnUTIL_ClientPrint(pPlayerController, 3, buf,nullptr, nullptr, nullptr, nullptr);
	}

	g_PlayerSkins[steamid][weaponId].m_nFallbackPaintKit = atoi(args.Arg(1));
	g_PlayerSkins[steamid][weaponId].m_nFallbackSeed = atoi(args.Arg(2));
	g_PlayerSkins[steamid][weaponId].m_flFallbackWear = atof(args.Arg(3));
	CBasePlayerWeapon* pPlayerWeapon = pWeaponServices->m_hActiveWeapon();

	pWeaponServices->RemoveWeapon(pPlayerWeapon);
	FnEntityRemove(g_pGameEntitySystem,pPlayerWeapon,nullptr,-1);
	FnGiveNamedItem(pPlayerPawn->m_pItemServices(),weapon_name->second.c_str(),nullptr,nullptr,nullptr,nullptr);
	// pPlayerWeapon->m_AttributeManager().m_Item().m_iAccountID() = 271098320;
	//CCSPlayer_ItemServices* pItemServices = static_cast<CCSPlayer_ItemServices*>(pPlayerPawn->m_pItemServices());
	//pItemServices->GiveNamedItem(weapon_name->second.c_str());
	// g_pGameRules->PlayerRespawn(static_cast<CCSPlayerPawn*>(pPlayerPawn));
	//META_CONPRINTF( "called by %lld\n", steamid);
	sprintf(buf, " \x04 [SKIN] \x01已修改皮肤 编号:%d 模板:%d 磨损:%f",g_PlayerSkins[steamid][weaponId].m_nFallbackPaintKit,g_PlayerSkins[steamid][weaponId].m_nFallbackSeed,g_PlayerSkins[steamid][weaponId].m_flFallbackWear);
	FnUTIL_ClientPrint(pPlayerController, 3, buf,nullptr, nullptr, nullptr, nullptr);
}

CON_COMMAND_F(i_subclass_change, "subclass change", FCVAR_NONE)
{
	FnSubClassChange(context,args);
}

const char* Skin::GetLicense()
{
	return "GPL";
}

const char* Skin::GetVersion()
{
	return "1.0.5";
}

const char* Skin::GetDate()
{
	return __DATE__;
}

const char* Skin::GetLogTag()
{
	return "skin";
}

const char* Skin::GetAuthor()
{
	return "宇宙遨游";
}

const char* Skin::GetDescription()
{
	return "武器皮肤插件";
}

const char* Skin::GetName()
{
	return "武器皮肤插件";
}

const char* Skin::GetURL()
{
	return "http://cs2.wssr.top";
}
