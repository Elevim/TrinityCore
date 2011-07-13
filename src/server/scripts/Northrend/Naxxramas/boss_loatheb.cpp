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

enum Spells
{
    SPELL_NECROTIC_AURA                                    = 55593,
    SPELL_SUMMON_SPORE                                     = 29234,
    SPELL_DEATHBLOOM                                       = 29865,
    H_SPELL_DEATHBLOOM                                     = 55053,
    SPELL_INEVITABLE_DOOM                                  = 29204,
    H_SPELL_INEVITABLE_DOOM                                = 55052
};

enum Events
{
    EVENT_NONE,
    EVENT_AURA,
    EVENT_BLOOM,
    EVENT_DOOM,
};

enum Achievments
{
    ACHIEV_SPORE_LOSER_10                                     = 2182,
    ACHIEV_SPORE_LOSER_25                                     = 2183,
};

bool isSporeKilled;
#define LOATHEB RAID_MODE(16011,29718)

class boss_loatheb : public CreatureScript
{
public:
    boss_loatheb() : CreatureScript("boss_loatheb") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_loathebAI (creature);
    }

    struct boss_loathebAI : public BossAI
    {
        boss_loathebAI(Creature* c) : BossAI(c, BOSS_LOATHEB) {}

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            events.ScheduleEvent(EVENT_AURA, 10000);
            events.ScheduleEvent(EVENT_BLOOM, 5000);
            events.ScheduleEvent(EVENT_DOOM, 120000);
            isSporeKilled = false;
        }

        void UpdateAI(const uint32 diff)
        {
            if (!UpdateVictim())
                return;

            events.Update(diff);

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_AURA:
                        DoCastAOE(SPELL_NECROTIC_AURA);
                        events.ScheduleEvent(EVENT_AURA, 20000);
                        break;
                    case EVENT_BLOOM:
                        // TODO : Add missing text
                        DoCastAOE(SPELL_SUMMON_SPORE, true);
                        DoCastAOE(RAID_MODE(SPELL_DEATHBLOOM, H_SPELL_DEATHBLOOM));
                        events.ScheduleEvent(EVENT_BLOOM, 30000);
                        break;
                    case EVENT_DOOM:
                        DoCastAOE(RAID_MODE(SPELL_INEVITABLE_DOOM, H_SPELL_INEVITABLE_DOOM));
                        events.ScheduleEvent(EVENT_DOOM, events.GetTimer() < 5*60000 ? 30000 : 15000);
                        break;
                }
            }

            DoMeleeAttackIfReady();
        }

        void KilledUnit(Unit* victim)
        {
            if(victim && victim->GetTypeId() == TYPEID_PLAYER)
                me->GetInstanceScript()->SetData(DATA_KILLED_PLAYER,1);
        }

        void JustDied(Unit* killer)
        {
            if(!isSporeKilled)
            {
                if (RAID_MODE(10, 25) == 25)
                    me->GetInstanceScript()->DoCompleteAchievement(ACHIEV_SPORE_LOSER_25);
                else
                    me->GetInstanceScript()->DoCompleteAchievement(ACHIEV_SPORE_LOSER_10);

            }

            me->GetInstanceScript()->SetBossState(BOSS_LOATHEB, DONE);
        }
    };

};

enum SporeSpells
{
    SPELL_FUNGAL_CREEP                                     = 29232
};

static Position locCenter = { 2910.24f, -3995.78f, 274.15f, 0.0f };


class mob_loatheb_spore: public CreatureScript
{
public:
    mob_loatheb_spore() : CreatureScript("mob_loatheb_spore") {}

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_loatheb_sporeAI(creature);
    }

    struct mob_loatheb_sporeAI : public ScriptedAI
    {
        mob_loatheb_sporeAI(Creature* creature) : ScriptedAI(creature)
        {
            pInstance = me->GetInstanceScript();

            MoveToLoatheb();    
        }

        InstanceScript* pInstance;
        std::list<Unit*> unitList;
        std::list<Unit*>::iterator players;
        uint32 deathTimer;


        void Reset()
        {
            me->SetSpeed(MOVE_RUN, 0.4f, true);
            deathTimer = 90000;
            me->SetReactState(REACT_PASSIVE);
        }

        void MoveToLoatheb()
        {
            me->GetMotionMaster()->MoveIdle();

            if (pInstance)
            {
                if (Unit* pLoatheb = me->FindNearestCreature(LOATHEB, 1000.0f, true))
                    me->GetMotionMaster()->MoveFollow(pLoatheb, 0.0f, 0.0f);
            }
        }


        void UpdateAI(const uint32 uiDiff)
        {
            if(!UpdateVictim())
                return;

            if (Unit* pLoatheb = me->FindNearestCreature(LOATHEB, 10.0f))
                me->Kill(me, false);

            if (deathTimer <= uiDiff)
            {
                deathTimer = 0;
                me->Kill(me,false);
            }else deathTimer -= uiDiff;

        }

        void JustDied(Unit* pKiller)
        {
            if (pKiller->GetTypeId() == TYPEID_PLAYER)
            {
                if (deathTimer != 0)
                    isSporeKilled = true;

                Map* pMap = me->GetMap();
                if (pMap && pMap->IsDungeon())
                {
                    pKiller->GetPartyMemberInDist(unitList, 15.0f);

                    for(players  = unitList.begin(); players != unitList.end(); ++players)
                        if ((*players)->ToPlayer()->IsInRange(me, 0.0F, 15.0f, true))
                        {
                            me->AddAura(SPELL_FUNGAL_CREEP, *players);
                        }
                }

            }

        }
    };

};

void AddSC_boss_loatheb()
{
    new boss_loatheb();
    new mob_loatheb_spore();
}
