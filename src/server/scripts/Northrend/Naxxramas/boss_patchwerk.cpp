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

/* ************* *
 * Living Poisen *
 * ************* */

#define SPELL_DEATH        5
#define NAXXMAP         533

class naxx_mob_poisen : public CreatureScript
{
public:
    naxx_mob_poisen() : CreatureScript("naxx_mob_poisen") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new naxx_mob_poisenAI (pCreature);
    }

    struct naxx_mob_poisenAI : public ScriptedAI
    {
        naxx_mob_poisenAI(Creature* c) : ScriptedAI(c) {}

    void Reset()
    {
         me->SetSpeed(MOVE_RUN, 0.4f, true);
    }

    void MoveInLineOfSight(Unit *pWho)
    {
        if (!pWho)
            return;

        if (uint32 pMap = me->GetMapId())
            if (pMap != NAXXMAP)
                return;

        if (me->IsWithinDistInMap(pWho, 1.0f))
        {
            if (pWho->GetTypeId() != TYPEID_PLAYER)
                return;

            pWho->CastSpell(pWho,SPELL_DEATH, true);
            me->ForcedDespawn();
        }
    }

    void UpdateAI(uint32 const uiDiff)
    {
        if (!UpdateVictim())
                return;

    }
    };
};


enum Spells
{
    SPELL_HATEFUL_STRIKE                        = 41926,
    H_SPELL_HATEFUL_STRIKE                      = 59192,
    SPELL_FRENZY                                = 28131,
    SPELL_BERSERK                               = 26662,
    SPELL_SLIME_BOLT                            = 32309,
};

enum Yells
{
    SAY_AGGRO_1                                 = -1533017,
    SAY_AGGRO_2                                 = -1533018,
    SAY_SLAY                                    = -1533019,
    SAY_DEATH                                   = -1533020,
    EMOTE_BERSERK                               = -1533021,
    EMOTE_ENRAGE                                = -1533022,
};

enum Events
{
    EVENT_NONE,
    EVENT_BERSERK,
    EVENT_HATEFUL,
    EVENT_SLIME
};

enum
{
    ACHIEV_MAKE_QUICK_WERK_OF_HIM_STARTING_EVENT  = 10286,
};

class boss_patchwerk : public CreatureScript
{
public:
    boss_patchwerk() : CreatureScript("boss_patchwerk") { }

    CreatureAI* GetAI(Creature* pCreature) const
    {
        return new boss_patchwerkAI (pCreature);
    }

    struct boss_patchwerkAI : public BossAI
    {
        boss_patchwerkAI(Creature *c) : BossAI(c, BOSS_PATCHWERK) {}

        bool Enraged;

        void Reset()
        {
            _Reset();

            if (instance)
                instance->DoStopTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_MAKE_QUICK_WERK_OF_HIM_STARTING_EVENT);
        }

        void KilledUnit(Unit* /*Victim*/)
        {
            if (!(rand()%5))
                DoScriptText(SAY_SLAY, me);
        }

        void JustDied(Unit* /*Killer*/)
        {
            _JustDied();
            DoScriptText(SAY_DEATH, me);
        }

        void EnterCombat(Unit * /*who*/)
        {
            _EnterCombat();
            Enraged = false;
            DoScriptText(RAND(SAY_AGGRO_1, SAY_AGGRO_2), me);
            events.ScheduleEvent(EVENT_HATEFUL, 1200);
            events.ScheduleEvent(EVENT_BERSERK, 360000);
            me->CallForHelp(1000.0f);
            if (instance)
                instance->DoStartTimedAchievement(ACHIEVEMENT_TIMED_TYPE_EVENT, ACHIEV_MAKE_QUICK_WERK_OF_HIM_STARTING_EVENT);
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
                    case EVENT_HATEFUL:
                    {
                        //Cast Hateful strike on the player with the highest
                        //amount of HP within melee distance
                        uint32 MostHP = 0;
                        Unit* pMostHPTarget = NULL;
                        std::list<HostileReference*>::const_iterator i = me->getThreatManager().getThreatList().begin();
                        for (; i != me->getThreatManager().getThreatList().end(); ++i)
                        {
                            Unit *pTarget = (*i)->getTarget();
                            if (pTarget->isAlive() && pTarget != me->getVictim() && pTarget->GetHealth() > MostHP && me->IsWithinMeleeRange(pTarget))
                            {
                                MostHP = pTarget->GetHealth();
                                pMostHPTarget = pTarget;
                            }
                        }

                        if (!pMostHPTarget)
                            pMostHPTarget = me->getVictim();

                        DoCast(pMostHPTarget, RAID_MODE(SPELL_HATEFUL_STRIKE, H_SPELL_HATEFUL_STRIKE), true);

                        events.ScheduleEvent(EVENT_HATEFUL, 1200);
                        break;
                    }
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK, true);
                        DoScriptText(EMOTE_BERSERK, me);
                        events.ScheduleEvent(EVENT_SLIME, 2000);
                        break;
                    case EVENT_SLIME:
                        DoCast(me->getVictim(), SPELL_SLIME_BOLT);
                        events.ScheduleEvent(EVENT_SLIME, 2000);
                        break;
                }
            }

            if (!Enraged && HealthBelowPct(5))
            {
                DoCast(me, SPELL_FRENZY, true);
                DoScriptText(EMOTE_ENRAGE, me);
                Enraged = true;
            }

            DoMeleeAttackIfReady();
        }
    };

};

void AddSC_boss_patchwerk()
{
    new boss_patchwerk();
    new naxx_mob_poisen();
}
