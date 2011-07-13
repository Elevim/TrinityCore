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

#define SPELL_BOMBARD_SLIME         28280

#define SPELL_POISON_CLOUD          28240
#define SPELL_MUTATING_INJECTION    28169
#define SPELL_SLIME_SPRAY           RAID_MODE(28157, 54364)
#define SPELL_BERSERK               26662
#define SPELL_POISON_CLOUD_ADD      59116

#define SPELL_POISON_CLOUD_HACK     30914
#define SPELL_AOE_NATURE_DMG        30915

#define EVENT_BERSERK   1
#define EVENT_CLOUD     2
#define EVENT_INJECT    3
#define EVENT_SPRAY     4

#define MOB_FALLOUT_SLIME   16290

class boss_grobbulus : public CreatureScript
{
public:
    boss_grobbulus() : CreatureScript("boss_grobbulus") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_grobbulusAI (creature);
    }

    struct boss_grobbulusAI : public BossAI
    {
        boss_grobbulusAI(Creature* c) : BossAI(c, BOSS_GROBBULUS)
        {
            me->ApplySpellImmune(0, IMMUNITY_ID, SPELL_POISON_CLOUD_ADD, true);
        }

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            me->CallForHelp(1500.0f);
            events.ScheduleEvent(EVENT_CLOUD, 15000);
            events.ScheduleEvent(EVENT_INJECT, 20000);
            events.ScheduleEvent(EVENT_SPRAY, 15000+rand()%15000); //not sure
            events.ScheduleEvent(EVENT_BERSERK, 12*60000);
        }

        void SpellHitTarget(Unit* target, const SpellEntry *spell)
        {
            if (spell->Id == uint32(SPELL_SLIME_SPRAY))
            {
                if (TempSummon *slime = me->SummonCreature(MOB_FALLOUT_SLIME, target->GetPositionX(), target->GetPositionY(), target->GetPositionZ(), 0, TEMPSUMMON_TIMED_DESPAWN_OUT_OF_COMBAT, 0))
                    DoZoneInCombat(slime);
            }
        }

        void KilledUnit(Unit* victim)
        {
            if(victim && victim->GetTypeId() == TYPEID_PLAYER)
                me->GetInstanceScript()->SetData(DATA_KILLED_PLAYER,1);
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
                    case EVENT_CLOUD:
                        DoCastAOE(SPELL_POISON_CLOUD);
                        events.ScheduleEvent(EVENT_CLOUD, 15000);
                        return;
                    case EVENT_BERSERK:
                        DoCastAOE(SPELL_BERSERK);
                        return;
                    case EVENT_SPRAY:
                        DoCastAOE(SPELL_SLIME_SPRAY);
                        events.ScheduleEvent(EVENT_SPRAY, 15000+rand()%15000);
                        return;
                    case EVENT_INJECT:
                        if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 200, true))
                            if (!target->HasAura(SPELL_MUTATING_INJECTION))
                                DoCast(target, SPELL_MUTATING_INJECTION);

                        events.ScheduleEvent(EVENT_INJECT, 8000 + uint32(120 * me->GetHealthPct()));
                        return;
                }
            }

            DoMeleeAttackIfReady();
        }
    };

};

class npc_grobbulus_poison_cloud : public CreatureScript
{
public:
    npc_grobbulus_poison_cloud() : CreatureScript("npc_grobbulus_poison_cloud") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_grobbulus_poison_cloudAI(creature);
    }

    struct npc_grobbulus_poison_cloudAI : public Scripted_NoMovementAI
    {
        npc_grobbulus_poison_cloudAI(Creature* creature) : Scripted_NoMovementAI(creature)
        {
            Reset();
        }

        uint32 Damage_Timer;
        uint32 Disappear_Timer;


        void Reset()
        {
            Damage_Timer = 1000;
            Disappear_Timer = 65000;
            SetCombatMovement(false);
            me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NON_ATTACKABLE);
            me->SetFloatValue(OBJECT_FIELD_SCALE_X, 1.85f);
            if (SpellEntry* TempSpell = (SpellEntry*)GetSpellStore()->LookupEntry(SPELL_POISON_CLOUD_HACK))
            {
                TempSpell->EffectTriggerSpell[0] = 204;
                me->CastCustomSpell(me, SPELL_POISON_CLOUD_HACK, NULL, NULL, NULL, true);
                me->CastCustomSpell(me, SPELL_POISON_CLOUD_HACK, NULL, NULL, NULL, true);
            }
        }

        void UpdateAI(const uint32 diff)
        {
            if (Damage_Timer < diff)
            {
                uint8 RadiusIndex;
                switch (uint32(Disappear_Timer / 4300))
                {
                    case 0:
                    case 1:  RadiusIndex = 43; break;
                    case 2:  RadiusIndex = 18; break;
                    case 3:  RadiusIndex = 61; break;
                    case 4:  RadiusIndex = 17; break;
                    case 5:  RadiusIndex = 32; break;
                    case 6:  RadiusIndex = 42; break;
                    case 7:  RadiusIndex = 13; break;
                    case 8:  RadiusIndex = 40; break;
                    case 9:  RadiusIndex = 14; break;
                    case 10: RadiusIndex = 37; break;
                    case 11: RadiusIndex = 29;  break;
                    case 12: RadiusIndex = 8; break;
                    case 13: RadiusIndex = 26; break;
                    case 14:
                    case 15: RadiusIndex = 15;  break;
                }
                if (SpellEntry* TempSpell = (SpellEntry*)GetSpellStore()->LookupEntry(SPELL_AOE_NATURE_DMG))
                {
                    TempSpell->EffectRadiusIndex[0] = RadiusIndex;
                    TempSpell->EffectBasePoints[0] = (RAID_MODE (2000, 4250));
                    me->CastCustomSpell(me, SPELL_AOE_NATURE_DMG, NULL, NULL, NULL, true);
                }
                Damage_Timer = 1000;
            }else Damage_Timer -= diff;

            if (Disappear_Timer < diff)
            {
                me->ForcedDespawn();
            }else Disappear_Timer -= diff;
        }
    };

};

void AddSC_boss_grobbulus()
{
    new boss_grobbulus();
    new npc_grobbulus_poison_cloud();
}
