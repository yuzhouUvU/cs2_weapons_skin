#include "db.h"
#include <map>

extern ISmmAPI *g_SMAPI;
extern std::map<uint64_t, std::map<int, SkinParm>> g_PlayerSkins;
extern std::map<uint64_t, int> g_PlayerKnifes;

DB::DB()
{
    g_SMAPI->PathFormat(buf, sizeof(buf), "%s/skin.db3", g_SMAPI->GetBaseDir());
    sqlite3_open(buf, &db);
    sqlite3_exec(db, "create table if not exists weapon(steamid integer,weaponid integer,paintkit integer not null,seed integer not null,wear real not null,constraint weapon_pk primary key (steamid,weaponid));create table if not exists knife(steamid integer,knifeid integer,constraint knife_pk primary key (steamid));", NULL, NULL, &sql3_errmsg);
    CheckErrMSG();
}

DB::~DB()
{
    sqlite3_close(db);
}

void DB::CheckErrMSG()
{
    if (sql3_errmsg)
    {
        META_CONPRINTF("\nSkin %s\n", sql3_errmsg);
        sqlite3_free(sql3_errmsg);
    }
}

void DB::QueryData(uint64_t steamid)
{
    sqlite3_stmt *stmt;

    // Query weapon table
    sprintf(buf, "SELECT weaponid,paintkit,seed,wear FROM weapon WHERE steamid=%lu;", steamid);
    std::map<int, SkinParm>& playerSkinMap = g_PlayerSkins[steamid];
    sqlite3_prepare(db, buf, sizeof(buf), &stmt, NULL);
    while (sqlite3_step(stmt) == SQLITE_ROW)
    {
        const int weaponid = sqlite3_column_int(stmt, 0);
        playerSkinMap[weaponid].m_nFallbackPaintKit = sqlite3_column_int(stmt, 1);
        playerSkinMap[weaponid].m_nFallbackSeed = sqlite3_column_int(stmt, 2);
        playerSkinMap[weaponid].m_flFallbackWear = static_cast<float>(sqlite3_column_double(stmt, 3));
    }
    sqlite3_finalize(stmt);

    // Query knife table
    sprintf(buf, "SELECT knifeid FROM knife WHERE steamid=%lu;", steamid);
    sqlite3_prepare(db, buf, sizeof(buf), &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW)
    {
        int knifeid = sqlite3_column_int(stmt, 0);
        g_PlayerKnifes[steamid] = knifeid;
    }
    sqlite3_finalize(stmt);
}

void DB::UpdateWeapon(uint64_t steamid, int weaponid, int paintKit, int seed, float wear)
{
    sprintf(buf, "INSERT OR REPLACE INTO weapon(steamid, weaponid, paintkit, seed, wear) VALUES(%lu, %d, %d, %d, %f);", steamid, weaponid, paintKit, seed, wear);
    sqlite3_exec(db, buf, NULL, NULL, &sql3_errmsg);
    CheckErrMSG();
}

void DB::UpdateKnife(uint64_t steamid, int knifeid)
{
    sprintf(buf, "INSERT OR REPLACE INTO knife(steamid, knifeid) VALUES(%lu, %d);", steamid, knifeid);
    sqlite3_exec(db, buf, NULL, NULL, &sql3_errmsg);
    CheckErrMSG();
}