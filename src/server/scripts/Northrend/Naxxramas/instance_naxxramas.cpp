/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE. See the GNU General Public License for
 * more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "ScriptPCH.h"
#include "naxxramas.h"

// Slime lane spawns
struct Location
{
    float x,y,z;
};

 static Location SlimeLane1Spawn = { 3140.50f, -3121.59f, 293.342f };
 static Location SlimeLane1Direction = { 3134.92f, -3157.50f, 293.477f };
 static Location SlimeLane2Spawn = { 3150.56f, -3123.39f, 293.334f };
 static Location SlimeLane2Direction = { 3143.87f, -3159.23f, 293.523f };
 static Location SlimeLane3Spawn = { 3160.44f, -3125.83f, 293.328f };
 static Location SlimeLane3Direction = { 3152.22f, -3161.43f, 293.505f };

const DoorData doorData[] =
{
    {181126,    BOSS_ANUBREKHAN, DOOR_TYPE_ROOM,     BOUNDARY_S},
    {181195,    BOSS_ANUBREKHAN, DOOR_TYPE_PASSAGE,  0},
    {194022,    BOSS_FAERLINA,   DOOR_TYPE_PASSAGE,  0},
    {181209,    BOSS_FAERLINA,   DOOR_TYPE_PASSAGE,  0},
    {181209,    BOSS_MAEXXNA,    DOOR_TYPE_ROOM,     BOUNDARY_SW},
    {181200,    BOSS_NOTH,       DOOR_TYPE_ROOM,     BOUNDARY_N},
    {181201,    BOSS_NOTH,       DOOR_TYPE_PASSAGE,  BOUNDARY_E},
    {181202,    BOSS_NOTH,       DOOR_TYPE_PASSAGE,  0},
    {181202,    BOSS_HEIGAN,     DOOR_TYPE_ROOM,     BOUNDARY_N},
    {181203,    BOSS_HEIGAN,     DOOR_TYPE_PASSAGE,  BOUNDARY_E},
    {181241,    BOSS_HEIGAN,     DOOR_TYPE_PASSAGE,  0},
    {181241,    BOSS_LOATHEB,    DOOR_TYPE_ROOM,     BOUNDARY_W},
    {181123,    BOSS_PATCHWERK,  DOOR_TYPE_PASSAGE,  0},
    {181123,    BOSS_GROBBULUS,  DOOR_TYPE_ROOM,     0},
    {181120,    BOSS_GLUTH,      DOOR_TYPE_PASSAGE,  BOUNDARY_NW},
    {181121,    BOSS_GLUTH,      DOOR_TYPE_PASSAGE,  0},
    {181121,    BOSS_THADDIUS,   DOOR_TYPE_ROOM,     0},
    {181124,    BOSS_RAZUVIOUS,  DOOR_TYPE_PASSAGE,  0},
    {181124,    BOSS_GOTHIK,     DOOR_TYPE_ROOM,     BOUNDARY_N},
    {181125,    BOSS_GOTHIK,     DOOR_TYPE_PASSAGE,  BOUNDARY_S},
    {181119,    BOSS_GOTHIK,     DOOR_TYPE_PASSAGE,  0},
    {181119,    BOSS_HORSEMEN,   DOOR_TYPE_ROOM,     BOUNDARY_NE},
    {181225,    BOSS_SAPPHIRON,  DOOR_TYPE_PASSAGE,  BOUNDARY_W},
    {181228,    BOSS_KELTHUZAD,  DOOR_TYPE_ROOM,     BOUNDARY_S},
    {0,         0,               DOOR_TYPE_ROOM,     0}, // EOF
};

const MinionData minionData[] =
{
    //{16573,     BOSS_ANUBREKHAN},     there is no spawn point in db, so we do not add them here
    {16506,     BOSS_FAERLINA},
    {16803,     BOSS_RAZUVIOUS},
    {16063,     BOSS_HORSEMEN},
    {16064,     BOSS_HORSEMEN},
    {16065,     BOSS_HORSEMEN},
    {30549,     BOSS_HORSEMEN},
    {0,         0, }
};

enum eEnums
{
    GO_HORSEMEN_CHEST_HERO  = 193426,
    GO_HORSEMEN_CHEST       = 181366,                   //four horsemen event, DoRespawnGameObject() when event == DONE
    GO_GOTHIK_GATE          = 181170,
    GO_KELTHUZAD_PORTAL01   = 181402,
    GO_KELTHUZAD_PORTAL02   = 181403,
    GO_KELTHUZAD_PORTAL03   = 181404,
    GO_KELTHUZAD_PORTAL04   = 181405,
    GO_KELTHUZAD_TRIGGER    = 181444,

    GO_PORTAL_MAEXXNA        = 181575,
    GO_PORTAL_THADDIUS        = 181576,
    GO_PORTAL_LOATHEB        = 181577,
    GO_PORTAL_4HORSE        = 181578,

    SPELL_ERUPTION          = 29371,

    NPC_POISEN                = 16027,
    NPC_LANE1                = 1000000,
    NPC_LANE2                = 1000001,
    NPC_LANE3                = 1000002
};

const float HeiganPos[2] = {2796, -3707};
const float HeiganEruptionSlope[3] =
{
    (-3685 - HeiganPos[1]) /(2724 - HeiganPos[0]),
    (-3647 - HeiganPos[1]) /(2749 - HeiganPos[0]),
    (-3637 - HeiganPos[1]) /(2771 - HeiganPos[0]),
};

// 0  H      x
//  1        ^
//   2       |
//    3  y<--o
inline uint32 GetEruptionSection(float x, float y)
{
    y -= HeiganPos[1];
    if (y < 1.0f)
        return 0;

    x -= HeiganPos[0];
    if (x > -1.0f)
        return 3;

    float slope = y/x;
    for (uint32 i = 0; i < 3; ++i)
        if (slope > HeiganEruptionSlope[i])
            return i;
    return 3;
}

class instance_naxxramas : public InstanceMapScript
{
public:
    instance_naxxramas() : InstanceMapScript("instance_naxxramas", 533) { }

    InstanceScript* GetInstanceScript(InstanceMap* pMap) const
    {
        return new instance_naxxramas_InstanceMapScript(pMap);
    }

    struct instance_naxxramas_InstanceMapScript : public InstanceScript
    {
        instance_naxxramas_InstanceMapScript(Map* pMap) : InstanceScript(pMap)
        {
            SetBossNumber(MAX_BOSS_NUMBER);
            LoadDoorData(doorData);
            LoadMinionData(minionData);
        }

        std::set<uint64> HeiganEruptionGUID[4];
        uint64 GothikGateGUID;
        uint64 HorsemenChestGUID;
        uint64 SapphironGUID;
        uint64 uiFaerlina;
        uint64 uiThane;
        uint64 uiLady;
        uint64 uiBaron;
        uint64 uiSir;

        uint64 uiThaddius;
        uint64 uiFeugen;
        uint64 uiStalagg;

        uint64 uiKelthuzad;
        uint64 uiKelthuzadTrigger;
        uint64 uiPortals[4];

        GOState gothikDoorState;

        time_t minHorsemenDiedTime;
        time_t maxHorsemenDiedTime;

        uint64 PortalMaexxnaGUID;
        uint64 PortalThaddiusGUID;
        uint64 PortalLoathebGUID;
        uint64 Portal4HorseGUID;

        uint64 uiLane1GUID;
        uint64 uiLane2GUID;
        uint64 uiLane3GUID;

        uint32 Lane1Timer;
        uint32 Lane2Timer;
        uint32 Lane3Timer;
        
        void Initialize()
        {
            uiLane1GUID = 0;
            uiLane2GUID = 0;
            uiLane3GUID = 0;

            Lane1Timer = 1000;
            Lane2Timer = 3000;
            Lane3Timer = 5000;
        }

        void OnCreatureCreate(Creature* creature)
        {
            switch(creature->GetEntry())
            {
                case 15989: SapphironGUID = creature->GetGUID(); return;
                case 15953: uiFaerlina = creature->GetGUID(); return;
                case 16064: uiThane = creature->GetGUID(); return;
                case 16065: uiLady = creature->GetGUID(); return;
                case 30549: uiBaron = creature->GetGUID(); return;
                case 16063: uiSir = creature->GetGUID(); return;
                case 15928: uiThaddius = creature->GetGUID(); return;
                case 15930: uiFeugen = creature->GetGUID(); return;
                case 15929: uiStalagg = creature->GetGUID(); return;
                case 15990: uiKelthuzad = creature->GetGUID(); return;
                case NPC_LANE1: uiLane1GUID =creature->GetGUID(); return;
                case NPC_LANE2: uiLane2GUID =creature->GetGUID(); return;
                case NPC_LANE3: uiLane3GUID =creature->GetGUID(); return;
            }

            AddMinion(creature, true);
        }

        void OnCreatureRemove(Creature* creature)
        {
            AddMinion(creature, false);
        }

        void OnGameObjectCreate(GameObject* go)
        {
            if (go->GetGOInfo()->displayId == 6785 || go->GetGOInfo()->displayId == 1287)
            {
                uint32 section = GetEruptionSection(go->GetPositionX(), go->GetPositionY());
                HeiganEruptionGUID[section].insert(go->GetGUID());

                return;
            }

            switch (go->GetEntry())
            {
                case GO_GOTHIK_GATE:
                    GothikGateGUID = go->GetGUID();
                    go->SetGoState(gothikDoorState);
                    break;
                case GO_HORSEMEN_CHEST:
                    HorsemenChestGUID = go->GetGUID();
                    break;
                case GO_HORSEMEN_CHEST_HERO:
                    HorsemenChestGUID = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL01:
                    uiPortals[0] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL02:
                    uiPortals[1] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL03:
                    uiPortals[2] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_PORTAL04:
                    uiPortals[3] = go->GetGUID();
                    break;
                case GO_KELTHUZAD_TRIGGER:
                    uiKelthuzadTrigger = go->GetGUID();
                    break;
                case GO_PORTAL_MAEXXNA:
                    PortalMaexxnaGUID = go->GetGUID();
                    break;
                case GO_PORTAL_THADDIUS:
                    PortalThaddiusGUID = go->GetGUID();
                    break;
                case GO_PORTAL_LOATHEB:
                    PortalLoathebGUID = go->GetGUID();
                    break;
                case GO_PORTAL_4HORSE:
                    Portal4HorseGUID = go->GetGUID();
                    break;
                default:
                    break;
            }

            AddDoor(go, true);
        }

        void OnGameObjectRemove(GameObject* go)
        {
            if (go->GetGOInfo()->displayId == 6785 || go->GetGOInfo()->displayId == 1287)
            {
                uint32 section = GetEruptionSection(go->GetPositionX(), go->GetPositionY());

                HeiganEruptionGUID[section].erase(go->GetGUID());
                return;
            }

            switch (go->GetEntry())
            {
                case GO_BIRTH:
                    if (SapphironGUID)
                    {
                        if (Creature* pSapphiron = instance->GetCreature(SapphironGUID))
                            pSapphiron->AI()->DoAction(DATA_SAPPHIRON_BIRTH);
                        return;
                    }
                    break;
                default:
                    break;
            }

            AddDoor(go, false);
        }

        void SetData(uint32 id, uint32 value)
        {
            switch(id)
            {
                case DATA_HEIGAN_ERUPT:
                    HeiganErupt(value);
                    break;
                case DATA_GOTHIK_GATE:
                    if (GameObject* gothikGate = instance->GetGameObject(GothikGateGUID))
                        gothikGate->SetGoState(GOState(value));
                    gothikDoorState = GOState(value);
                    break;

                case DATA_HORSEMEN0:
                case DATA_HORSEMEN1:
                case DATA_HORSEMEN2:
                case DATA_HORSEMEN3:
                    if (value == NOT_STARTED)
                    {
                        minHorsemenDiedTime = 0;
                        maxHorsemenDiedTime = 0;
                    }
                    else if (value == DONE)
                    {
                        time_t now = time(NULL);

                        if (minHorsemenDiedTime == 0)
                            minHorsemenDiedTime = now;

                        maxHorsemenDiedTime = now;
                    }
                    break; 
            }
        }

        uint64 GetData64(uint32 id)
        {
            switch(id)
            {
            case DATA_FAERLINA:
                return uiFaerlina;
            case DATA_THANE:
                return uiThane;
            case DATA_LADY:
                return uiLady;
            case DATA_BARON:
                return uiBaron;
            case DATA_SIR:
                return uiSir;
            case DATA_THADDIUS:
                return uiThaddius;
            case DATA_FEUGEN:
                return uiFeugen;
            case DATA_STALAGG:
                return uiStalagg;
            case DATA_KELTHUZAD:
                return uiKelthuzad;
            case DATA_KELTHUZAD_PORTAL01:
                return uiPortals[0];
            case DATA_KELTHUZAD_PORTAL02:
                return uiPortals[1];
            case DATA_KELTHUZAD_PORTAL03:
                return uiPortals[2];
            case DATA_KELTHUZAD_PORTAL04:
                return uiPortals[3];
            case DATA_KELTHUZAD_TRIGGER:
                return uiKelthuzadTrigger;
            case DATA_LANE1:
                return uiLane1GUID;
            case DATA_LANE2:
                return uiLane2GUID;
            case DATA_LANE3:
                return uiLane3GUID;
            }
            return 0;
        }

        bool SetBossState(uint32 id, EncounterState state)
        {
            if (!InstanceScript::SetBossState(id, state))
                return false;

            if (id == BOSS_HORSEMEN && state == DONE)
            {
                if (GameObject* pHorsemenChest = instance->GetGameObject(HorsemenChestGUID))
                    pHorsemenChest->SetRespawnTime(pHorsemenChest->GetRespawnDelay());

                if (GameObject* pHorsePortal = instance->GetGameObject(Portal4HorseGUID))
                    pHorsePortal->SetPhaseMask(1, true);
            }

            if (id == BOSS_THADDIUS && state == DONE)
            {
                if (GameObject* pThaddiusPortal = instance->GetGameObject(PortalThaddiusGUID))
                    pThaddiusPortal->SetPhaseMask(1, true);
            }
            
            if (id == BOSS_MAEXXNA && state == DONE)
            {
                if (GameObject* pMaexxnaPortal = instance->GetGameObject(PortalMaexxnaGUID))
                    pMaexxnaPortal->SetPhaseMask(1, true);
            }

            if (id == BOSS_LOATHEB && state == DONE)
            {
                if (GameObject* pLoathebPortal = instance->GetGameObject(PortalLoathebGUID))
                    pLoathebPortal->SetPhaseMask(1, true);
            }

            return true;
        }

        void HeiganErupt(uint32 section)
        {
            for (uint32 i = 0; i < 4; ++i)
            {
                if (i == section)
                    continue;

                for (std::set<uint64>::const_iterator itr = HeiganEruptionGUID[i].begin(); itr != HeiganEruptionGUID[i].end(); ++itr)
                {
                    if (GameObject* pHeiganEruption = instance->GetGameObject(*itr))
                    {
                        pHeiganEruption->SendCustomAnim(pHeiganEruption->GetGoAnimProgress());
                        pHeiganEruption->CastSpell(NULL, SPELL_ERUPTION);
                    }
                }
            }
        }

        void Update(uint32 uiDiff)
        {
            if (Lane1Timer < uiDiff)
            {
                if(Creature* pTrigger = instance->GetCreature(GetData64(DATA_LANE1)))
                    if (Creature* pTemp = pTrigger->SummonCreature(NPC_POISEN, SlimeLane1Spawn.x, SlimeLane1Spawn.y, SlimeLane1Spawn.z, 4.636764f, TEMPSUMMON_TIMED_DESPAWN, 12000))
                        pTemp->GetMotionMaster()->MovePoint(0, SlimeLane1Direction.x, SlimeLane1Direction.y, SlimeLane1Direction.z);
                Lane1Timer = 5500;
            }else Lane1Timer -= uiDiff; 

            if (Lane2Timer < uiDiff)
            {
                if(Creature* pTrigger = instance->GetCreature(GetData64(DATA_LANE2)))
                    if (Creature* pTemp = pTrigger->SummonCreature(NPC_POISEN, SlimeLane2Spawn.x, SlimeLane2Spawn.y, SlimeLane2Spawn.z, 4.636764f, TEMPSUMMON_TIMED_DESPAWN, 12000))
                        pTemp->GetMotionMaster()->MovePoint(0, SlimeLane2Direction.x, SlimeLane2Direction.y, SlimeLane2Direction.z);
                Lane2Timer = 5500;
            }else Lane2Timer -= uiDiff; 

            if (Lane3Timer < uiDiff)
            {
                if(Creature* pTrigger = instance->GetCreature(GetData64(DATA_LANE3)))
                    if (Creature* pTemp = pTrigger->SummonCreature(NPC_POISEN, SlimeLane3Spawn.x, SlimeLane3Spawn.y, SlimeLane3Spawn.z, 4.636764f, TEMPSUMMON_TIMED_DESPAWN, 12000))
                        pTemp->GetMotionMaster()->MovePoint(0, SlimeLane3Direction.x, SlimeLane3Direction.y, SlimeLane3Direction.z);
                Lane3Timer = 5500;
            }else Lane3Timer -= uiDiff;
        }

        bool CheckAchievementCriteriaMeet(uint32 criteria_id, Player const* /*source*/, Unit const* /*target = NULL*/, uint32 /*miscvalue1 = 0*/)
        {
            switch(criteria_id)
            {
                case 7600:  // Criteria for achievement 2176: And They Would All Go Down Together 15sec of each other 10-man
                    if (Difficulty(instance->GetSpawnMode()) == RAID_DIFFICULTY_10MAN_NORMAL && (maxHorsemenDiedTime - minHorsemenDiedTime) < 15)
                        return true;
                    return false;
                case 7601:  // Criteria for achievement 2177: And They Would All Go Down Together 15sec of each other 25-man
                    if (Difficulty(instance->GetSpawnMode()) == RAID_DIFFICULTY_25MAN_NORMAL && (maxHorsemenDiedTime - minHorsemenDiedTime) < 15)
                        return true;
                    return false;
                case 13233: // Criteria for achievement 2186: The Immortal (25-man)
                    // TODO.
                    break;
                case 13237: // Criteria for achievement 2187: The Undying (10-man)
                    // TODO.
                    break;
            }
            return false;
        }

        std::string GetSaveData()
        {
            std::ostringstream saveStream;
            saveStream << GetBossSaveData() << " " << gothikDoorState;
            return saveStream.str();
        }

        void Load(const char * data)
        {
            std::istringstream loadStream(LoadBossState(data));
            uint32 buff;
            loadStream >> buff;
            gothikDoorState = GOState(buff);
        }
    };

};

class at_naxxramas_frostwyrm_wing : public AreaTriggerScript
{
    public:
        at_naxxramas_frostwyrm_wing() : AreaTriggerScript("at_naxxramas_frostwyrm_wing") { }

    bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
    {
        if (player->isGameMaster())
            return false;

        InstanceScript* instance = player->GetInstanceScript();
        if (instance)
        {
            for (uint32 i = BOSS_ANUBREKHAN; i < BOSS_SAPPHIRON; ++i)
            {
                if (instance->GetBossState(i) != DONE)
                    return true;
            }
            return false;
        }

        return true;
    }
};


/* Naxxramas Portal */

#define NaxxMap          533
#define NaxxX            2957.200928f       
#define NaxxY            -3434.486084f
#define NaxxZ            299.550476f
#define NaxxO            6.256170f

class go_naxx_portal : public GameObjectScript
{
    public:
        go_naxx_portal() : GameObjectScript("go_naxx_portal") { }

        bool OnGossipHello(Player* pPlayer, GameObject* pGo)
        {
            InstanceScript* pInstance = pPlayer->GetInstanceScript();

            if (!pInstance)
                return false;
            
            if (pPlayer)
            {
               pPlayer->TeleportTo(NaxxMap, NaxxX, NaxxY, NaxxZ, NaxxO, true);
            }

            return true;
        }
};


void AddSC_instance_naxxramas()
{
    new instance_naxxramas();
    new at_naxxramas_frostwyrm_wing();
    new go_naxx_portal();
}
