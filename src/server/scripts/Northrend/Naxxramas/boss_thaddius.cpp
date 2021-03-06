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

//Stalagg
enum StalaggYells
{
    SAY_STAL_AGGRO                      = -1533023, //not used
    SAY_STAL_SLAY                       = -1533024, //not used
    SAY_STAL_DEATH                      = -1533025  //not used
};

enum StalagSpells
{
    SPELL_POWERSURGE                    = 28134,
    H_SPELL_POWERSURGE                  = 54529,
    SPELL_MAGNETIC_PULL                 = 28338,
    SPELL_STALAGG_TESLA                 = 28097
};

//Feugen
enum FeugenYells
{
    SAY_FEUG_AGGRO                      = -1533026, //not used
    SAY_FEUG_SLAY                       = -1533027, //not used
    SAY_FEUG_DEATH                      = -1533028  //not used
};

enum FeugenSpells
{
    SPELL_STATICFIELD                   = 28135,
    H_SPELL_STATICFIELD                 = 54528,
    SPELL_FEUGEN_TESLA                  = 28109
};

// Thaddius DoAction
enum ThaddiusActions
{
    ACTION_FEUGEN_RESET,
    ACTION_FEUGEN_DIED,
    ACTION_STALAGG_RESET,
    ACTION_STALAGG_DIED
};

//generic
#define C_TESLA_COIL                    16218           //the coils (emotes "Tesla Coil overloads!")

//Tesla Spells (deals damage to the group if Stalagg or Feugen are too far away)
enum TeslaSpells
{
    SPELL_SHOCK                         = 28099,
    SPELL_STALAGG_CHAIN                 = 28096,
    SPELL_STALAGG_TESLA_PASSIVE         = 28097,
    SPELL_FEUGEN_TESLA_PASSIVE          = 28109,
    SPELL_FEUGEN_CHAIN                  = 28111
};

//Thaddius
enum ThaddiusYells
{
    SAY_GREET                           = -1533029, //not used
    SAY_AGGRO_1                         = -1533030,
    SAY_AGGRO_2                         = -1533031,
    SAY_AGGRO_3                         = -1533032,
    SAY_SLAY                            = -1533033,
    SAY_ELECT                           = -1533034, //not used
    SAY_DEATH                           = -1533035,
    SAY_SCREAM1                         = -1533036, //not used
    SAY_SCREAM2                         = -1533037, //not used
    SAY_SCREAM3                         = -1533038, //not used
    SAY_SCREAM4                         = -1533039 //not used
};

enum ThaddiusSpells
{
    SPELL_POLARITY_SHIFT                = 28089,
    SPELL_BALL_LIGHTNING                = 28299,
    SPELL_CHAIN_LIGHTNING               = 28167,
    H_SPELL_CHAIN_LIGHTNING             = 54531,
    SPELL_BERSERK                       = 27680
};

enum Events
{
    EVENT_NONE,
    EVENT_SHIFT,
    EVENT_CHAIN,
    EVENT_BERSERK,
};

class boss_thaddius : public CreatureScript
{
public:
    boss_thaddius() : CreatureScript("boss_thaddius") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new boss_thaddiusAI (creature);
    }

    struct boss_thaddiusAI : public BossAI
    {
        boss_thaddiusAI(Creature* c) : BossAI(c, BOSS_THADDIUS)
        {
            // init is a bit tricky because thaddius shall track the life of both adds, but not if there was a wipe
            // and, in particular, if there was a crash after both adds were killed (should not respawn)

            // Moreover, the adds may not yet be spawn. So just track down the status if mob is spawn
            // and each mob will send its status at reset (meaning that it is alive)
            checkFeugenAlive = false;
            if (Creature* pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                checkFeugenAlive = pFeugen->isAlive();

            checkStalaggAlive = false;
            if (Creature* pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
                checkStalaggAlive = pStalagg->isAlive();

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_AGGRESSIVE);
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_PASSIVE);
            }
            me->ApplySpellImmune(0, IMMUNITY_EFFECT, SPELL_EFFECT_KNOCK_BACK, true);
            me->ApplySpellImmune(0, IMMUNITY_MECHANIC, MECHANIC_GRIP, true);
        }

        bool checkStalaggAlive;
        bool checkFeugenAlive;
        uint32 uiAddsTimer;

        void Reset()
        {
            if (instance)
            {
                if (Creature* pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
                    pStalagg->Respawn();
                if (Creature* pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                    pFeugen->Respawn();
            }
        }

        void KilledUnit(Unit* victim)
        {
            if (!(rand()%5))
                DoScriptText(SAY_SLAY, me);

            if(victim && victim->GetTypeId() == TYPEID_PLAYER)
                me->GetInstanceScript()->SetData(DATA_KILLED_PLAYER,1);
        }

        void JustDied(Unit* /*Killer*/)
        {
            _JustDied();
            DoScriptText(SAY_DEATH, me);
            if(me->GetInstanceScript())
                me->GetInstanceScript()->SetBossState(BOSS_THADDIUS, DONE);
        }

        void DoAction(const int32 action)
        {
            switch(action)
            {
                case ACTION_FEUGEN_RESET:
                    checkFeugenAlive = true;
                    break;
                case ACTION_FEUGEN_DIED:
                    checkFeugenAlive = false;
                    break;
                case ACTION_STALAGG_RESET:
                    checkStalaggAlive = true;
                    break;
                case ACTION_STALAGG_DIED:
                    checkStalaggAlive = false;
                    break;
            }

            if (!checkFeugenAlive && !checkStalaggAlive)
            {
                me->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                // REACT_AGGRESSIVE only reset when he takes damage.
                DoZoneInCombat();
            }
            else
            {
                me->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_OOC_NOT_ATTACKABLE | UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_STUNNED);
                me->SetReactState(REACT_PASSIVE);
            }
        }

        void EnterCombat(Unit* /*who*/)
        {
            _EnterCombat();
            DoScriptText(RAND(SAY_AGGRO_1, SAY_AGGRO_2, SAY_AGGRO_3), me);
            events.ScheduleEvent(EVENT_SHIFT, 30000);
            events.ScheduleEvent(EVENT_CHAIN, urand(10000, 20000));
            events.ScheduleEvent(EVENT_BERSERK, 360000);
        }

        void DamageTaken(Unit* /*pDoneBy*/, uint32 & /*uiDamage*/)
        {
            me->SetReactState(REACT_AGGRESSIVE);
        }

        void UpdateAI(const uint32 diff)
        {
            if (checkFeugenAlive && checkStalaggAlive)
                uiAddsTimer = 0;

            if (checkStalaggAlive != checkFeugenAlive)
            {
                uiAddsTimer += diff;
                if (uiAddsTimer > 5000)
                {
                    if (!checkStalaggAlive)
                    {
                        if (instance)
                            if (Creature* pStalagg = me->GetCreature(*me, instance->GetData64(DATA_STALAGG)))
                                pStalagg->Respawn();
                    }
                    else
                    {
                        if (instance)
                            if (Creature* pFeugen = me->GetCreature(*me, instance->GetData64(DATA_FEUGEN)))
                                pFeugen->Respawn();
                    }
                }
            }

            if (!UpdateVictim())
                return;

            events.Update(diff);

            if (me->HasUnitState(UNIT_STAT_CASTING))
                return;

            while (uint32 eventId = events.ExecuteEvent())
            {
                switch(eventId)
                {
                    case EVENT_SHIFT:
                        DoCastAOE(SPELL_POLARITY_SHIFT);
                        events.ScheduleEvent(EVENT_SHIFT, 30000);
                        return;
                    case EVENT_CHAIN:
                        DoCast(me->getVictim(), RAID_MODE(SPELL_CHAIN_LIGHTNING, H_SPELL_CHAIN_LIGHTNING));
                        events.ScheduleEvent(EVENT_CHAIN, urand(10000, 20000));
                        return;
                    case EVENT_BERSERK:
                        DoCast(me, SPELL_BERSERK);
                        return;
                }
            }

            if (events.GetTimer() > 15000 && !me->IsWithinMeleeRange(me->getVictim()))
                DoCast(me->getVictim(), SPELL_BALL_LIGHTNING);
            else
                DoMeleeAttackIfReady();
        }
    };

};

class mob_stalagg : public CreatureScript
{
public:
    mob_stalagg() : CreatureScript("mob_stalagg") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_stalaggAI(creature);
    }

    struct mob_stalaggAI : public ScriptedAI
    {
        mob_stalaggAI(Creature* c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
        }

        InstanceScript* pInstance;

        uint32 powerSurgeTimer;
        uint32 magneticPullTimer;
        uint32 ChainTimer;
        uint32 ShockTimer;
        uint32 IdleTimer;
        uint64 TeslaGuid;
        bool bChainReset;
        bool bShock;
        bool bSwitch;
        Position homePosition;

        void Reset()
        {
            if (pInstance)
                if (Creature* pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_STALAGG_RESET);

            powerSurgeTimer = urand(20000, 25000);
            magneticPullTimer = 20000;
            IdleTimer = 3000;
            ShockTimer = 1000;
            ChainTimer = 1000;

            bChainReset = true;
            bShock = false;
            bSwitch = false;
            homePosition = me->GetHomePosition();
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_STALAGG_TESLA);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (pInstance)
                if (Creature* pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_STALAGG_DIED);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (bChainReset)
            {
                if (ChainTimer <= uiDiff)
                {
                    if (Creature *pTesla = me->FindNearestCreature(C_TESLA_COIL, 50))
                    {
                        TeslaGuid = pTesla->GetGUID();
                        pTesla->CastSpell(me, SPELL_STALAGG_CHAIN, false);
                    }
                    bChainReset = false;
                    ChainTimer = 3000;
                } else ChainTimer -= uiDiff;
            }

            if (!UpdateVictim())
                return;

            if (magneticPullTimer <= uiDiff)
            {
                if (Creature* pFeugen = me->GetCreature(*me, pInstance->GetData64(DATA_FEUGEN)))
                {
                    Unit* pStalaggVictim = me->getVictim();
                    Unit* pFeugenVictim = pFeugen->getVictim();

                    if (pFeugenVictim && pStalaggVictim)
                    {
                        // magnetic pull is not working. So just jump.
                        // reset aggro to be sure that feugen will not follow the jump
                        // switch aggro, tank gets generated thread from the other tank
                        float uiTempThreat = pFeugen->getThreatManager().getThreat(pFeugenVictim);
                        pFeugen->getThreatManager().modifyThreatPercent(pFeugenVictim, -100);
                        pFeugenVictim->JumpTo(me, 0.3f);
                        pFeugen->AddThreat(pStalaggVictim, uiTempThreat);
                        pFeugen->SetReactState(REACT_PASSIVE);

                        uiTempThreat = me->getThreatManager().getThreat(pStalaggVictim);
                        me->getThreatManager().modifyThreatPercent(pStalaggVictim, -100);
                        pStalaggVictim->JumpTo(pFeugen, 0.3f);
                        me->AddThreat(pFeugenVictim, uiTempThreat);
                        me->SetReactState(REACT_PASSIVE);
                        IdleTimer = 3000;
                        bSwitch = true;
                    }
                }

                magneticPullTimer = 20000;
            }
            else magneticPullTimer -= uiDiff;

            if (powerSurgeTimer <= uiDiff)
            {
                DoCast(me, RAID_MODE(SPELL_POWERSURGE, H_SPELL_POWERSURGE));
                powerSurgeTimer = urand(15000, 20000);
            } else powerSurgeTimer -= uiDiff;

            if (bSwitch)
            if (IdleTimer <= uiDiff)
            {
                if (Creature *pFeugen = me->GetCreature(*me, pInstance->GetData64(DATA_FEUGEN)))
                    pFeugen->SetReactState(REACT_AGGRESSIVE);
                me->SetReactState(REACT_AGGRESSIVE);
                bSwitch = false;
            } else IdleTimer -= uiDiff;

            if (me->GetDistance(homePosition) > 15)
            {
                if (ShockTimer <= uiDiff)
                {
                    if (Creature *pTesla = Creature::GetCreature(*me, TeslaGuid))
                        if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 50000, true))
                            pTesla->CastSpell(pTarget, SPELL_SHOCK, true);
                    ShockTimer = 1000;
                    bShock = true;
                }else ShockTimer -= uiDiff;
            }
            else if (bShock)
            {
                bShock = false;
                bChainReset = true;
            }
            DoMeleeAttackIfReady();
        }
    };

};

class mob_feugen : public CreatureScript
{
public:
    mob_feugen() : CreatureScript("mob_feugen") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_feugenAI(creature);
    }

    struct mob_feugenAI : public ScriptedAI
    {
        mob_feugenAI(Creature* c) : ScriptedAI(c)
        {
            pInstance = c->GetInstanceScript();
        }

        InstanceScript* pInstance;

        uint32 staticFieldTimer;
        uint32 ShockTimer;
        uint64 TeslaGuid;
        uint32 ChainTimer;
        bool bChainReset;
        bool bShock;
        Position homePosition;

        void Reset()
        {
            if (pInstance)
                if (Creature* pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_FEUGEN_RESET);

            staticFieldTimer = 5000;
            ShockTimer = 1000;
            ChainTimer = 10000;
            bChainReset = true;
            bShock = false;
            homePosition = me->GetHomePosition();
        }

        void EnterCombat(Unit* /*who*/)
        {
            DoCast(SPELL_FEUGEN_TESLA);
        }

        void JustDied(Unit* /*killer*/)
        {
            if (pInstance)
                if (Creature* pThaddius = me->GetCreature(*me, pInstance->GetData64(DATA_THADDIUS)))
                    if (pThaddius->AI())
                        pThaddius->AI()->DoAction(ACTION_FEUGEN_DIED);
        }

        void UpdateAI(const uint32 uiDiff)
        {
            if (bChainReset)
            {
                if (ChainTimer <= uiDiff)
                {
                    if (Creature *pTesla = me->FindNearestCreature(C_TESLA_COIL, 50))
                    {
                        TeslaGuid = pTesla->GetGUID();
                        pTesla->CastSpell(me, SPELL_FEUGEN_CHAIN, false);
                    }
                    bChainReset = false;
                    ChainTimer = 3000;
                } else ChainTimer -= uiDiff;
            }

            if (!UpdateVictim())
                return;

            if (staticFieldTimer <= uiDiff)
            {
                DoCast(me, RAID_MODE(SPELL_STATICFIELD, H_SPELL_STATICFIELD));
                staticFieldTimer = 5000;
            } else staticFieldTimer -= uiDiff;

            if (me->GetDistance(homePosition) > 15)
            {
                if (ShockTimer <= uiDiff)
                {
                    if (Creature *pTesla = Creature::GetCreature(*me, TeslaGuid))
                        if (Unit *pTarget = SelectTarget(SELECT_TARGET_RANDOM, 0, 50000, true))
                            pTesla->CastSpell(pTarget, SPELL_SHOCK, false);
                    ShockTimer = 1000;
                    bShock = true;
                }else ShockTimer -= uiDiff;
            }
            else if (bShock)
            {
                bShock = false;
                bChainReset = true;
            }
            DoMeleeAttackIfReady();
        }
    };

};

void AddSC_boss_thaddius()
{
    new boss_thaddius();
    new mob_stalagg();
    new mob_feugen();
}
