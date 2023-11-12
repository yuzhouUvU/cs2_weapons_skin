#include "sqlite3/sqlite3.h"
#include "Skin.h"
#include <map>

class DB
{
private:
    sqlite3 *db;
    char buf[2048];
    char *sql3_errmsg = nullptr;
    void CheckErrMSG();

public:
    DB();
    ~DB();
    void QueryData(uint64_t steamid);
    void UpdateWeapon(uint64_t steamid, int weaponid, int paintKit, int seed, float wear);
    void UpdateKnife(uint64_t steamid, int knifeid);
};