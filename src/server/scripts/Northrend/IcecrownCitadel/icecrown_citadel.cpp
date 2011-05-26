/*
 * Copyright (C) 2008-2010 TrinityCore <http://www.trinitycore.org/>
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

#include "ObjectMgr.h"
#include "ScriptMgr.h"
#include "ScriptedCreature.h"
#include "SpellAuraEffects.h"
#include "icecrown_citadel.h"

// Weekly quest support
//* Deprogramming                (DONE)
//* Securing the Ramparts        (DONE)
//* Residue Rendezvous           (DONE)
//* Blood Quickening             (DONE)
//* Respite for a Tormented Soul

enum Texts
{
    // Highlord Tirion Fordring (at Light's Hammer)
    SAY_TIRION_INTRO_1              = 0,
    SAY_TIRION_INTRO_2              = 1,
    SAY_TIRION_INTRO_3              = 2,
    SAY_TIRION_INTRO_4              = 3,
    SAY_TIRION_INTRO_H_5            = 4,
    SAY_TIRION_INTRO_A_5            = 5,

    // The Lich King (at Light's Hammer)
    SAY_LK_INTRO_1                  = 0,
    SAY_LK_INTRO_2                  = 1,
    SAY_LK_INTRO_3                  = 2,
    SAY_LK_INTRO_4                  = 3,
    SAY_LK_INTRO_5                  = 4,

    // Highlord Bolvar Fordragon (at Light's Hammer)
    SAY_BOLVAR_INTRO_1              = 0,

    // High Overlord Saurfang (at Light's Hammer)
    SAY_SAURFANG_INTRO_1            = 15,
    SAY_SAURFANG_INTRO_2            = 16,
    SAY_SAURFANG_INTRO_3            = 17,
    SAY_SAURFANG_INTRO_4            = 18,

    // Muradin Bronzebeard (at Light's Hammer)
    SAY_MURADIN_INTRO_1             = 13,
    SAY_MURADIN_INTRO_2             = 14,
    SAY_MURADIN_INTRO_3             = 15,

    // Rotting Frost Giant
    EMOTE_DEATH_PLAGUE_WARNING      = 0,
};

enum Spells
{
    // Rotting Frost Giant
    SPELL_DEATH_PLAGUE              = 72879,
    SPELL_DEATH_PLAGUE_AURA         = 72865,
    SPELL_RECENTLY_INFECTED         = 72884,
    SPELL_DEATH_PLAGUE_KILL         = 72867,
    SPELL_STOMP                     = 64652,
    SPELL_ARCTIC_BREATH             = 72848,

    // Frost Freeze Trap
    SPELL_COLDFLAME_JETS            = 70460,

    // Alchemist Adrianna
    SPELL_HARVEST_BLIGHT_SPECIMEN   = 72155,
    SPELL_HARVEST_BLIGHT_SPECIMEN25 = 72162,
};

enum Events
{
    // Highlord Tirion Fordring (at Light's Hammer)
    // The Lich King (at Light's Hammer)
    // Highlord Bolvar Fordragon (at Light's Hammer)
    // High Overlord Saurfang (at Light's Hammer)
    // Muradin Bronzebeard (at Light's Hammer)
    EVENT_TIRION_INTRO_2    = 1,
    EVENT_TIRION_INTRO_3    = 2,
    EVENT_TIRION_INTRO_4    = 3,
    EVENT_TIRION_INTRO_5    = 4,
    EVENT_LK_INTRO_1        = 5,
    EVENT_TIRION_INTRO_6    = 6,
    EVENT_LK_INTRO_2        = 7,
    EVENT_LK_INTRO_3        = 8,
    EVENT_LK_INTRO_4        = 9,
    EVENT_BOLVAR_INTRO_1    = 10,
    EVENT_LK_INTRO_5        = 11,
    EVENT_SAURFANG_INTRO_1  = 12,
    EVENT_TIRION_INTRO_H_7  = 13,
    EVENT_SAURFANG_INTRO_2  = 14,
    EVENT_SAURFANG_INTRO_3  = 15,
    EVENT_SAURFANG_INTRO_4  = 16,
    EVENT_SAURFANG_RUN      = 17,
    EVENT_MURADIN_INTRO_1   = 18,
    EVENT_MURADIN_INTRO_2   = 19,
    EVENT_MURADIN_INTRO_3   = 20,
    EVENT_TIRION_INTRO_A_7  = 21,
    EVENT_MURADIN_INTRO_4   = 22,
    EVENT_MURADIN_INTRO_5   = 23,
    EVENT_MURADIN_RUN       = 24,

    // Rotting Frost Giant
    EVENT_DEATH_PLAGUE      = 25,
    EVENT_STOMP             = 26,
    EVENT_ARCTIC_BREATH     = 27,

    // Frost Freeze Trap
    EVENT_ACTIVATE_TRAP     = 28,
};

enum DataTypesICC
{
    DATA_DAMNED_KILLS       = 1,
};


enum Actions
{
    // Sister Svalna
    ACTION_KILL_CAPTAIN         = 1,
    ACTION_START_GAUNTLET       = 2,
    ACTION_RESURRECT_CAPTAINS   = 3,
    ACTION_CAPTAIN_DIES         = 4,
    ACTION_RESET_EVENT          = 5,
};

enum EventIds
{
    EVENT_AWAKEN_WARD_1 = 22900,
    EVENT_AWAKEN_WARD_2 = 22907,
    EVENT_AWAKEN_WARD_3 = 22908,
    EVENT_AWAKEN_WARD_4 = 22909,
};

class FrostwingVrykulSearcher
{
    public:
        FrostwingVrykulSearcher(Creature const* source, float range) : _source(source), _range(range) {}

        bool operator()(Unit* unit)
        {
            if (!unit->isAlive())
                return false;

            switch (unit->GetEntry())
            {
                case NPC_YMIRJAR_BATTLE_MAIDEN:
                case NPC_YMIRJAR_DEATHBRINGER:
                case NPC_YMIRJAR_FROSTBINDER:
                case NPC_YMIRJAR_HUNTRESS:
                case NPC_YMIRJAR_WARLORD:
                    break;
                default:
                    return false;
            }

            if (!unit->IsWithinDist(_source, _range, false))
                return false;

            return true;
        }

    private:
        Creature const* _source;
        float _range;
};

class FrostwingGauntletRespawner
{
    public:
        void operator()(Creature* creature)
        {
            switch (creature->GetOriginalEntry())
            {
                case NPC_YMIRJAR_BATTLE_MAIDEN:
                case NPC_YMIRJAR_DEATHBRINGER:
                case NPC_YMIRJAR_FROSTBINDER:
                case NPC_YMIRJAR_HUNTRESS:
                case NPC_YMIRJAR_WARLORD:
                    break;
                case NPC_CROK_SCOURGEBANE:
                case NPC_CAPTAIN_ARNATH:
                case NPC_CAPTAIN_BRANDON:
                case NPC_CAPTAIN_GRONDEL:
                case NPC_CAPTAIN_RUPERT:
                    creature->AI()->DoAction(ACTION_RESET_EVENT);
                    break;
                case NPC_SISTER_SVALNA:
                    creature->AI()->DoAction(ACTION_RESET_EVENT);
                    // return, this creature is never dead if event is reset
                    return;
                default:
                    return;
            }

            uint32 corpseDelay = creature->GetCorpseDelay();
            uint32 respawnDelay = creature->GetRespawnDelay();
            creature->SetCorpseDelay(1);
            creature->SetRespawnDelay(2);

            if (CreatureData const* data = creature->GetCreatureData())
                creature->SetPosition(data->posX, data->posY, data->posZ, data->orientation);
            creature->ForcedDespawn();

            creature->SetCorpseDelay(corpseDelay);
            creature->SetRespawnDelay(respawnDelay);
        }
};

class CaptainSurviveTalk : public BasicEvent
{
    public:
        explicit CaptainSurviveTalk(Creature const& owner) : _owner(owner) { }

        bool Execute(uint64 /*currTime*/, uint32 /*diff*/)
        {
            _owner.AI()->Talk(SAY_CAPTAIN_SURVIVE_TALK);
            return true;
        }

    private:
        Creature const& _owner;
};

>>>>>>> 84ab669... Scripts/Icecrown Citadel: Fixed Deathbound wards & their trap linking
// at Light's Hammer
class npc_highlord_tirion_fordring_lh : public CreatureScript
{
    public:
        npc_highlord_tirion_fordring_lh() : CreatureScript("npc_highlord_tirion_fordring_lh") { }

        struct npc_highlord_tirion_fordringAI : public ScriptedAI
        {
            npc_highlord_tirion_fordringAI(Creature* creature) : ScriptedAI(creature), instance(creature->GetInstanceScript())
            {
            }

            void Reset()
            {
                events.Reset();
                _theLichKing = 0;
                _bolvarFordragon = 0;
                _factionNPC = 0;
                _damnedKills = 0;
            }

            // IMPORTANT NOTE: This is triggered from per-GUID scripts
            // of The Damned SAI
            void SetData(uint32 type, uint32 data)
            {
                if (type == DATA_DAMNED_KILLS && data == 1)
                {
                    if (++_damnedKills == 2)
                    {
                        if (Creature* theLichKing = me->FindNearestCreature(NPC_THE_LICH_KING_LH, 150.0f))
                        {
                            if (Creature* bolvarFordragon = me->FindNearestCreature(NPC_HIGHLORD_BOLVAR_FORDRAGON_LH, 150.0f))
                            {
                                if (Creature* factionNPC = me->FindNearestCreature(instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE ? NPC_SE_HIGH_OVERLORD_SAURFANG : NPC_SE_MURADIN_BRONZEBEARD, 50.0f))
                                {
                                    me->setActive(true);
                                    _theLichKing = theLichKing->GetGUID();
                                    theLichKing->setActive(true);
                                    _bolvarFordragon = bolvarFordragon->GetGUID();
                                    bolvarFordragon->setActive(true);
                                    _factionNPC = factionNPC->GetGUID();
                                    factionNPC->setActive(true);
                                }
                            }
                        }

                        if (!_bolvarFordragon || !_theLichKing || !_factionNPC)
                            return;

                        Talk(SAY_TIRION_INTRO_1);
                        events.ScheduleEvent(EVENT_TIRION_INTRO_2, 4000);
                        events.ScheduleEvent(EVENT_TIRION_INTRO_3, 14000);
                        events.ScheduleEvent(EVENT_TIRION_INTRO_4, 18000);
                        events.ScheduleEvent(EVENT_TIRION_INTRO_5, 31000);
                        events.ScheduleEvent(EVENT_LK_INTRO_1, 35000);
                        events.ScheduleEvent(EVENT_TIRION_INTRO_6, 51000);
                        events.ScheduleEvent(EVENT_LK_INTRO_2, 58000);
                        events.ScheduleEvent(EVENT_LK_INTRO_3, 74000);
                        events.ScheduleEvent(EVENT_LK_INTRO_4, 86000);
                        events.ScheduleEvent(EVENT_BOLVAR_INTRO_1, 100000);
                        events.ScheduleEvent(EVENT_LK_INTRO_5, 108000);

                        if (instance->GetData(DATA_TEAM_IN_INSTANCE) == HORDE)
                        {
                            events.ScheduleEvent(EVENT_SAURFANG_INTRO_1, 120000);
                            events.ScheduleEvent(EVENT_TIRION_INTRO_H_7, 129000);
                            events.ScheduleEvent(EVENT_SAURFANG_INTRO_2, 139000);
                            events.ScheduleEvent(EVENT_SAURFANG_INTRO_3, 150000);
                            events.ScheduleEvent(EVENT_SAURFANG_INTRO_4, 162000);
                            events.ScheduleEvent(EVENT_SAURFANG_RUN, 170000);
                        }
                        else
                        {
                            events.ScheduleEvent(EVENT_MURADIN_INTRO_1, 120000);
                            events.ScheduleEvent(EVENT_MURADIN_INTRO_2, 124000);
                            events.ScheduleEvent(EVENT_MURADIN_INTRO_3, 127000);
                            events.ScheduleEvent(EVENT_TIRION_INTRO_A_7, 136000);
                            events.ScheduleEvent(EVENT_MURADIN_INTRO_4, 144000);
                            events.ScheduleEvent(EVENT_MURADIN_INTRO_5, 151000);
                            events.ScheduleEvent(EVENT_MURADIN_RUN, 157000);
                        }
                    }
                }
            }

            void UpdateAI(uint32 const diff)
            {
                if (_damnedKills != 2)
                    return;

                events.Update(diff);

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_TIRION_INTRO_2:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            break;
                        case EVENT_TIRION_INTRO_3:
                            Talk(SAY_TIRION_INTRO_2);
                            break;
                        case EVENT_TIRION_INTRO_4:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            break;
                        case EVENT_TIRION_INTRO_5:
                            Talk(SAY_TIRION_INTRO_3);
                            break;
                        case EVENT_LK_INTRO_1:
                            me->HandleEmoteCommand(EMOTE_ONESHOT_POINT_NOSHEATHE);
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_1);
                            break;
                        case EVENT_TIRION_INTRO_6:
                            Talk(SAY_TIRION_INTRO_4);
                            break;
                        case EVENT_LK_INTRO_2:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_2);
                            break;
                        case EVENT_LK_INTRO_3:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_3);
                            break;
                        case EVENT_LK_INTRO_4:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                                theLichKing->AI()->Talk(SAY_LK_INTRO_4);
                            break;
                        case EVENT_BOLVAR_INTRO_1:
                            if (Creature* bolvarFordragon = ObjectAccessor::GetCreature(*me, _bolvarFordragon))
                            {
                                bolvarFordragon->AI()->Talk(SAY_BOLVAR_INTRO_1);
                                bolvarFordragon->setActive(false);
                            }
                            break;
                        case EVENT_LK_INTRO_5:
                            if (Creature* theLichKing = ObjectAccessor::GetCreature(*me, _theLichKing))
                            {
                                theLichKing->AI()->Talk(SAY_LK_INTRO_5);
                                theLichKing->setActive(false);
                            }
                            break;
                        case EVENT_SAURFANG_INTRO_1:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_1);
                            break;
                        case EVENT_TIRION_INTRO_H_7:
                            Talk(SAY_TIRION_INTRO_H_5);
                            break;
                        case EVENT_SAURFANG_INTRO_2:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_2);
                            break;
                        case EVENT_SAURFANG_INTRO_3:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_3);
                            break;
                        case EVENT_SAURFANG_INTRO_4:
                            if (Creature* saurfang = ObjectAccessor::GetCreature(*me, _factionNPC))
                                saurfang->AI()->Talk(SAY_SAURFANG_INTRO_4);
                            break;
                        case EVENT_MURADIN_RUN:
                        case EVENT_SAURFANG_RUN:
                            if (Creature* factionNPC = ObjectAccessor::GetCreature(*me, _factionNPC))
                                factionNPC->GetMotionMaster()->MovePath(factionNPC->GetDBTableGUIDLow()*10, false);
                            me->setActive(false);
                            _damnedKills = 3;
                            break;
                        case EVENT_MURADIN_INTRO_1:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->AI()->Talk(SAY_MURADIN_INTRO_1);
                            break;
                        case EVENT_MURADIN_INTRO_2:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->HandleEmoteCommand(EMOTE_ONESHOT_TALK);
                            break;
                        case EVENT_MURADIN_INTRO_3:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->HandleEmoteCommand(EMOTE_ONESHOT_EXCLAMATION);
                            break;
                        case EVENT_TIRION_INTRO_A_7:
                            Talk(SAY_TIRION_INTRO_A_5);
                            break;
                        case EVENT_MURADIN_INTRO_4:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->AI()->Talk(SAY_MURADIN_INTRO_2);
                            break;
                        case EVENT_MURADIN_INTRO_5:
                            if (Creature* muradin = ObjectAccessor::GetCreature(*me, _factionNPC))
                                muradin->AI()->Talk(SAY_MURADIN_INTRO_3);
                            break;
                        default:
                            break;
                    }
                }
            }

        private:
            EventMap events;
            InstanceScript* const instance;
            uint64 _theLichKing;
            uint64 _bolvarFordragon;
            uint64 _factionNPC;
            uint16 _damnedKills;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_highlord_tirion_fordringAI>(creature);
        }
};

class npc_rotting_frost_giant : public CreatureScript
{
    public:
        npc_rotting_frost_giant() : CreatureScript("npc_rotting_frost_giant") { }

        struct npc_rotting_frost_giantAI : public ScriptedAI
        {
            npc_rotting_frost_giantAI(Creature* creature) : ScriptedAI(creature)
            {
            }

            void Reset()
            {
                events.Reset();
                events.ScheduleEvent(EVENT_DEATH_PLAGUE, 15000);
                events.ScheduleEvent(EVENT_STOMP, urand(5000, 8000));
                events.ScheduleEvent(EVENT_ARCTIC_BREATH, urand(10000, 15000));
            }

            void JustDied(Unit* /*killer*/)
            {
                events.Reset();
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_DEATH_PLAGUE:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                            {
                                Talk(EMOTE_DEATH_PLAGUE_WARNING, target->GetGUID());
                                DoCast(target, SPELL_DEATH_PLAGUE);
                            }
                            events.ScheduleEvent(EVENT_DEATH_PLAGUE, 15000);
                            break;
                        case EVENT_STOMP:
                            DoCastVictim(SPELL_STOMP);
                            events.ScheduleEvent(EVENT_STOMP, urand(15000, 18000));
                            break;
                        case EVENT_ARCTIC_BREATH:
                            DoCastVictim(SPELL_ARCTIC_BREATH);
                            events.ScheduleEvent(EVENT_ARCTIC_BREATH, urand(26000, 33000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_rotting_frost_giantAI>(creature);
        }
};

class npc_frost_freeze_trap : public CreatureScript
{
    public:
        npc_frost_freeze_trap() : CreatureScript("npc_frost_freeze_trap") { }

        struct npc_frost_freeze_trapAI: public Scripted_NoMovementAI
        {
            npc_frost_freeze_trapAI(Creature* creature) : Scripted_NoMovementAI(creature)
            {
            }

            void DoAction(int32 const action)
            {
                switch (action)
                {
                    case 1000:
                    case 11000:
                        events.ScheduleEvent(EVENT_ACTIVATE_TRAP, uint32(action));
                        break;
                    case ACTION_STOP_TRAPS:
                        me->RemoveAurasDueToSpell(SPELL_COLDFLAME_JETS);
                        events.CancelEvent(EVENT_ACTIVATE_TRAP);
                        break;
                    default:
                        break;
                }
            }

            void UpdateAI(uint32 const diff)
            {
                events.Update(diff);

                if (events.ExecuteEvent() == EVENT_ACTIVATE_TRAP)
                {
                    DoCast(me, SPELL_COLDFLAME_JETS);
                    events.ScheduleEvent(EVENT_ACTIVATE_TRAP, 22000);
                }
            }

        private:
            EventMap events;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_frost_freeze_trapAI>(creature);
        }
};

class npc_alchemist_adrianna : public CreatureScript
{
    public:
        npc_alchemist_adrianna() : CreatureScript("npc_alchemist_adrianna") { }

        bool OnGossipHello(Player* player, Creature* creature)
        {
            if (!creature->FindCurrentSpellBySpellId(SPELL_HARVEST_BLIGHT_SPECIMEN) && !creature->FindCurrentSpellBySpellId(SPELL_HARVEST_BLIGHT_SPECIMEN25))
                if (player->HasAura(SPELL_ORANGE_BLIGHT_RESIDUE) && player->HasAura(SPELL_GREEN_BLIGHT_RESIDUE))
                    creature->CastSpell(creature, SPELL_HARVEST_BLIGHT_SPECIMEN, false);
            return false;
        }
};

class DeathPlagueTargetSelector
{
    public:
        DeathPlagueTargetSelector(Unit* _caster) : caster(_caster) {}

        bool operator()(Unit* unit)
        {
            if (unit == caster)
                return true;

            if (unit->GetTypeId() != TYPEID_PLAYER)
                return true;

            if (unit->HasAura(SPELL_RECENTLY_INFECTED) || unit->HasAura(SPELL_DEATH_PLAGUE_AURA))
                return true;

            return false;
        }

        Unit* caster;
};

class spell_frost_giant_death_plague : public SpellScriptLoader
{
    public:
        spell_frost_giant_death_plague() : SpellScriptLoader("spell_frost_giant_death_plague") { }

        class spell_frost_giant_death_plague_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_frost_giant_death_plague_SpellScript);

            bool Load()
            {
                failed = false;
                return true;
            }

            // First effect
            void CountTargets(std::list<Unit*>& unitList)
            {
                unitList.remove(GetCaster());
                failed = unitList.empty();
            }

            // Second effect
            void FilterTargets(std::list<Unit*>& unitList)
            {
                // Select valid targets for jump
                unitList.remove_if(DeathPlagueTargetSelector(GetCaster()));
                if (!unitList.empty())
                {
                    std::list<Unit*>::iterator itr = unitList.begin();
                    std::advance(itr, urand(0, unitList.size()-1));
                    Unit* target = *itr;
                    unitList.clear();
                    unitList.push_back(target);
                }

                unitList.push_back(GetCaster());
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (GetHitUnit() != GetCaster())
                    GetCaster()->CastSpell(GetHitUnit(), SPELL_DEATH_PLAGUE_AURA, true);
                else if (failed)
                    GetCaster()->CastSpell(GetCaster(), SPELL_DEATH_PLAGUE_KILL, true);
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_frost_giant_death_plague_SpellScript::CountTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);
                OnUnitTargetSelect += SpellUnitTargetFn(spell_frost_giant_death_plague_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_AREA_ALLY_SRC);
                OnEffect += SpellEffectFn(spell_frost_giant_death_plague_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }

            bool failed;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_frost_giant_death_plague_SpellScript();
        }
};

class spell_icc_harvest_blight_specimen : public SpellScriptLoader
{
    public:
        spell_icc_harvest_blight_specimen() : SpellScriptLoader("spell_icc_harvest_blight_specimen") { }

        class spell_icc_harvest_blight_specimen_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icc_harvest_blight_specimen_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetHitUnit()->RemoveAurasDueToSpell(uint32(GetEffectValue()));
            }

            void HandleQuestComplete(SpellEffIndex /*effIndex*/)
            {
                GetHitUnit()->RemoveAurasDueToSpell(uint32(GetEffectValue()));
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_icc_harvest_blight_specimen_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnEffect += SpellEffectFn(spell_icc_harvest_blight_specimen_SpellScript::HandleQuestComplete, EFFECT_1, SPELL_EFFECT_QUEST_COMPLETE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icc_harvest_blight_specimen_SpellScript();
        }
};

class at_icc_saurfang_portal : public AreaTriggerScript
{
    public:
        at_icc_saurfang_portal() : AreaTriggerScript("at_icc_saurfang_portal") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            InstanceScript* instance = player->GetInstanceScript();
            if (!instance || instance->GetBossState(DATA_DEATHBRINGER_SAURFANG) != DONE)
                return true;

            player->TeleportTo(631, 4126.35f, 2769.23f, 350.963f, 0.0f);

            if (instance->GetData(DATA_COLDFLAME_JETS) == NOT_STARTED)
            {
                // Process relocation now, to preload the grid and initialize traps
                player->GetMap()->PlayerRelocation(player, 4126.35f, 2769.23f, 350.963f, 0.0f);

                instance->SetData(DATA_COLDFLAME_JETS, IN_PROGRESS);
                std::list<Creature*> traps;
                GetCreatureListWithEntryInGrid(traps, player, NPC_FROST_FREEZE_TRAP, 120.0f);
                traps.sort(Trinity::ObjectDistanceOrderPred(player));
                bool instant = false;
                for (std::list<Creature*>::iterator itr = traps.begin(); itr != traps.end(); ++itr)
                {
                    (*itr)->AI()->DoAction(instant ? 1000 : 11000);
                    instant = !instant;
                }
            }

            void UpdateEscortAI(uint32 const diff)
            {
                if (_wipeCheckTimer <= diff)
                    _wipeCheckTimer = 0;
                else
                    _wipeCheckTimer -= diff;

                if (!UpdateVictim() && !_isEventActive)
                    return;

                _events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = _events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ARNATH_INTRO_2:
                            if (Creature* arnath = ObjectAccessor::GetCreature(*me, _instance->GetData64(DATA_CAPTAIN_ARNATH)))
                                arnath->AI()->Talk(SAY_ARNATH_INTRO_2);
                            break;
                        case EVENT_CROK_INTRO_3:
                            Talk(SAY_CROK_INTRO_3);
                            break;
                        case EVENT_START_PATHING:
                            Start(true, true);
                            break;
                        case EVENT_SCOURGE_STRIKE:
                            DoCastVictim(SPELL_SCOURGE_STRIKE);
                            _events.ScheduleEvent(EVENT_SCOURGE_STRIKE, urand(10000, 14000));
                            break;
                        case EVENT_DEATH_STRIKE:
                            if (HealthBelowPct(20))
                                DoCastVictim(SPELL_DEATH_STRIKE);
                            _events.ScheduleEvent(EVENT_DEATH_STRIKE, urand(5000, 10000));
                            break;
                        case EVENT_HEALTH_CHECK:
                            if (HealthAbovePct(15))
                            {
                                me->RemoveAurasDueToSpell(SPELL_ICEBOUND_ARMOR);
                                _didUnderTenPercentText = false;
                            }
                            else
                            {
                                me->DealHeal(me, me->CountPctFromMaxHealth(5));
                                _events.ScheduleEvent(EVENT_HEALTH_CHECK, 1000);
                            }
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

            bool CanAIAttack(Unit const* target) const
            {
                // do not see targets inside Frostwing Halls when we are not there
                return (me->GetPositionY() > 2660.0f) == (target->GetPositionY() > 2660.0f);
            }

        private:
            EventMap _events;
            std::set<uint64> _aliveTrash;
            InstanceScript* _instance;
            uint32 _currentWPid;
            uint32 _wipeCheckTimer;
            uint32 const _respawnTime;
            uint32 const _corpseDelay;
            bool _isEventActive;
            bool _isEventDone;
            bool _didUnderTenPercentText;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_crok_scourgebaneAI>(creature);
        }
};

struct npc_argent_captainAI : public ScriptedAI
{
    public:
        npc_argent_captainAI(Creature* creature) : ScriptedAI(creature), Instance(creature->GetInstanceScript()), _firstDeath(true)
        {
            FollowAngle = PET_FOLLOW_ANGLE;
            FollowDist = PET_FOLLOW_DIST;
            IsUndead = false;
        }

        void JustDied(Unit* /*killer*/)
        {
            if (_firstDeath)
            {
                _firstDeath = false;
                Talk(SAY_CAPTAIN_DEATH);
            }
            else
                Talk(SAY_CAPTAIN_SECOND_DEATH);
        }

        void KilledUnit(Unit* victim)
        {
            if (victim->GetTypeId() == TYPEID_PLAYER)
                Talk(SAY_CAPTAIN_KILL);
        }

        void DoAction(int32 const action)
        {
            if (action == ACTION_START_GAUNTLET)
            {
                if (Creature* crok = ObjectAccessor::GetCreature(*me, Instance->GetData64(DATA_CROK_SCOURGEBANE)))
                {
                    me->SetReactState(REACT_DEFENSIVE);
                    FollowAngle = me->GetAngle(crok) + me->GetOrientation();
                    FollowDist = me->GetDistance2d(crok);
                    me->GetMotionMaster()->MoveFollow(crok, FollowDist, FollowAngle, MOTION_SLOT_IDLE);
                }

                me->setActive(true);
            }
            else if (action == ACTION_RESET_EVENT)
            {
                _firstDeath = true;
            }
        }

        void EnterCombat(Unit* /*target*/)
        {
            me->SetHomePosition(*me);
            if (IsUndead)
                DoZoneInCombat();
        }

        bool CanAIAttack(Unit const* target) const
        {
            // do not see targets inside Frostwing Halls when we are not there
            return (me->GetPositionY() > 2660.0f) == (target->GetPositionY() > 2660.0f);
        }

        void EnterEvadeMode()
        {
            // not yet following
            if (me->GetMotionMaster()->GetMotionSlotType(MOTION_SLOT_IDLE) != TARGETED_MOTION_TYPE || IsUndead)
            {
                ScriptedAI::EnterEvadeMode();
                return;
            }

            if (!_EnterEvadeMode())
                return;

            if (!me->GetVehicle())
            {
                me->GetMotionMaster()->Clear(false);
                if (Creature* crok = ObjectAccessor::GetCreature(*me, Instance->GetData64(DATA_CROK_SCOURGEBANE)))
                    me->GetMotionMaster()->MoveFollow(crok, FollowDist, FollowAngle, MOTION_SLOT_IDLE);
            }

            Reset();
        }

        void SpellHit(Unit* /*caster*/, SpellEntry const* spell)
        {
            if (spell->Id == SPELL_REVIVE_CHAMPION && !IsUndead)
            {
                IsUndead = true;
                me->setDeathState(JUST_ALIVED);
                uint32 newEntry = 0;
                switch (me->GetEntry())
                {
                    case NPC_CAPTAIN_ARNATH:
                        newEntry = NPC_CAPTAIN_ARNATH_UNDEAD;
                        break;
                    case NPC_CAPTAIN_BRANDON:
                        newEntry = NPC_CAPTAIN_BRANDON_UNDEAD;
                        break;
                    case NPC_CAPTAIN_GRONDEL:
                        newEntry = NPC_CAPTAIN_GRONDEL_UNDEAD;
                        break;
                    case NPC_CAPTAIN_RUPERT:
                        newEntry = NPC_CAPTAIN_RUPERT_UNDEAD;
                        break;
                    default:
                        return;
                }

                Talk(SAY_CAPTAIN_RESURRECTED);
                me->UpdateEntry(newEntry, Instance->GetData(DATA_TEAM_IN_INSTANCE), me->GetCreatureData());
                DoCast(me, SPELL_UNDEATH, true);
            }
        }

    protected:
        EventMap Events;
        InstanceScript* Instance;
        float FollowAngle;
        float FollowDist;
        bool IsUndead;

    private:
        bool _firstDeath;
};

class npc_captain_arnath : public CreatureScript
{
    public:
        npc_captain_arnath() : CreatureScript("npc_captain_arnath") { }

        struct npc_captain_arnathAI : public npc_argent_captainAI
        {
            npc_captain_arnathAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_ARNATH_FLASH_HEAL, urand(4000, 7000));
                Events.ScheduleEvent(EVENT_ARNATH_PW_SHIELD, urand(8000, 14000));
                Events.ScheduleEvent(EVENT_ARNATH_SMITE, urand(3000, 6000));
                if (Is25ManRaid() && IsUndead)
                    Events.ScheduleEvent(EVENT_ARNATH_DOMINATE_MIND, urand(22000, 27000));
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_ARNATH_FLASH_HEAL:
                            if (Creature* target = FindFriendlyCreature())
                                DoCast(target, SPELL_FLASH_HEAL);
                            Events.ScheduleEvent(EVENT_ARNATH_FLASH_HEAL, urand(6000, 9000));
                            break;
                        case EVENT_ARNATH_PW_SHIELD:
                        {
                            std::list<Creature*> targets = DoFindFriendlyMissingBuff(40.0f, SPELL_POWER_WORD_SHIELD);
                            std::list<Creature*>::iterator itr = targets.begin();
                            std::advance(itr, urand(0, targets.size() - 1));
                            DoCast(*itr, SPELL_POWER_WORD_SHIELD);
                            Events.ScheduleEvent(EVENT_ARNATH_PW_SHIELD, urand(15000, 20000));
                            break;
                        }
                        case EVENT_ARNATH_SMITE:
                            DoCastVictim(SPELL_SMITE);
                            Events.ScheduleEvent(EVENT_ARNATH_SMITE, urand(4000, 7000));
                            break;
                        case EVENT_ARNATH_DOMINATE_MIND:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_DOMINATE_MIND);
                            Events.ScheduleEvent(EVENT_ARNATH_DOMINATE_MIND, urand(28000, 37000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }

        private:
            Creature* FindFriendlyCreature() const
            {
                Creature* target = NULL;
                Trinity::MostHPMissingInRange u_check(me, 60.0f, 0);
                Trinity::CreatureLastSearcher<Trinity::MostHPMissingInRange> searcher(me, target, u_check);
                me->VisitNearbyGridObject(60.0f, searcher);
                return target;
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_arnathAI>(creature);
        }
};

class npc_captain_brandon : public CreatureScript
{
    public:
        npc_captain_brandon() : CreatureScript("npc_captain_brandon") { }

        struct npc_captain_brandonAI : public npc_argent_captainAI
        {
            npc_captain_brandonAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_BRANDON_CRUSADER_STRIKE, urand(6000, 10000));
                Events.ScheduleEvent(EVENT_BRANDON_DIVINE_SHIELD, 500);
                Events.ScheduleEvent(EVENT_BRANDON_JUDGEMENT_OF_COMMAND, urand(8000, 13000));
                if (IsUndead)
                    Events.ScheduleEvent(EVENT_BRANDON_HAMMER_OF_BETRAYAL, urand(25000, 30000));
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_BRANDON_CRUSADER_STRIKE:
                            DoCastVictim(SPELL_CRUSADER_STRIKE);
                            Events.ScheduleEvent(EVENT_BRANDON_CRUSADER_STRIKE, urand(6000, 12000));
                            break;
                        case EVENT_BRANDON_DIVINE_SHIELD:
                            if (HealthBelowPct(20))
                                DoCast(me, SPELL_DIVINE_SHIELD);
                            Events.ScheduleEvent(EVENT_BRANDON_DIVINE_SHIELD, 500);
                            break;
                        case EVENT_BRANDON_JUDGEMENT_OF_COMMAND:
                            DoCastVictim(SPELL_JUDGEMENT_OF_COMMAND);
                            Events.ScheduleEvent(EVENT_BRANDON_JUDGEMENT_OF_COMMAND, urand(8000, 13000));
                            break;
                        case EVENT_BRANDON_HAMMER_OF_BETRAYAL:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1, 0.0f, true))
                                DoCast(target, SPELL_HAMMER_OF_BETRAYAL);
                            Events.ScheduleEvent(EVENT_BRANDON_HAMMER_OF_BETRAYAL, urand(45000, 60000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_brandonAI>(creature);
        }
};

class npc_captain_grondel : public CreatureScript
{
    public:
        npc_captain_grondel() : CreatureScript("npc_captain_grondel") { }

        struct npc_captain_grondelAI : public npc_argent_captainAI
        {
            npc_captain_grondelAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_GRONDEL_CHARGE_CHECK, 500);
                Events.ScheduleEvent(EVENT_GRONDEL_MORTAL_STRIKE, urand(8000, 14000));
                Events.ScheduleEvent(EVENT_GRONDEL_SUNDER_ARMOR, urand(3000, 12000));
                if (IsUndead)
                    Events.ScheduleEvent(EVENT_GRONDEL_CONFLAGRATION, urand(12000, 17000));
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_GRONDEL_CHARGE_CHECK:
                            if (CanCast(me->getVictim(), sSpellStore.LookupEntry(SPELL_CHARGE)))
                                DoCastVictim(SPELL_CHARGE);
                            Events.ScheduleEvent(EVENT_GRONDEL_CHARGE_CHECK, 500);
                            break;
                        case EVENT_GRONDEL_MORTAL_STRIKE:
                            DoCastVictim(SPELL_MORTAL_STRIKE);
                            Events.ScheduleEvent(EVENT_GRONDEL_MORTAL_STRIKE, urand(10000, 15000));
                            break;
                        case EVENT_GRONDEL_SUNDER_ARMOR:
                            DoCastVictim(SPELL_SUNDER_ARMOR);
                            Events.ScheduleEvent(EVENT_GRONDEL_SUNDER_ARMOR, urand(5000, 17000));
                            break;
                        case EVENT_GRONDEL_CONFLAGRATION:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0, 0.0f, true))
                                DoCast(target, SPELL_CONFLAGRATION);
                            Events.ScheduleEvent(EVENT_GRONDEL_CONFLAGRATION, urand(10000, 15000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_grondelAI>(creature);
        }
};

class npc_captain_rupert : public CreatureScript
{
    public:
        npc_captain_rupert() : CreatureScript("npc_captain_rupert") { }

        struct npc_captain_rupertAI : public npc_argent_captainAI
        {
            npc_captain_rupertAI(Creature* creature) : npc_argent_captainAI(creature)
            {
            }

            void Reset()
            {
                Events.Reset();
                Events.ScheduleEvent(EVENT_RUPERT_FEL_IRON_BOMB, urand(15000, 20000));
                Events.ScheduleEvent(EVENT_RUPERT_MACHINE_GUN, urand(25000, 30000));
                Events.ScheduleEvent(EVENT_RUPERT_ROCKET_LAUNCH, urand(10000, 15000));
            }

            void UpdateAI(uint32 const diff)
            {
                if (!UpdateVictim())
                    return;

                Events.Update(diff);

                if (me->HasUnitState(UNIT_STAT_CASTING))
                    return;

                while (uint32 eventId = Events.ExecuteEvent())
                {
                    switch (eventId)
                    {
                        case EVENT_RUPERT_FEL_IRON_BOMB:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 0))
                                DoCast(target, SPELL_FEL_IRON_BOMB);
                            Events.ScheduleEvent(EVENT_RUPERT_FEL_IRON_BOMB, urand(15000, 20000));
                            break;
                        case EVENT_RUPERT_MACHINE_GUN:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                DoCast(target, SPELL_MACHINE_GUN);
                            Events.ScheduleEvent(EVENT_RUPERT_MACHINE_GUN, urand(25000, 30000));
                            break;
                        case EVENT_RUPERT_ROCKET_LAUNCH:
                            if (Unit* target = SelectTarget(SELECT_TARGET_RANDOM, 1))
                                DoCast(target, SPELL_ROCKET_LAUNCH);
                            Events.ScheduleEvent(EVENT_RUPERT_ROCKET_LAUNCH, urand(10000, 15000));
                            break;
                        default:
                            break;
                    }
                }

                DoMeleeAttackIfReady();
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return GetIcecrownCitadelAI<npc_captain_rupertAI>(creature);
        }
};

class npc_frostwing_vrykul : public CreatureScript
{
    public:
        npc_frostwing_vrykul() : CreatureScript("npc_frostwing_vrykul") { }

        struct npc_frostwing_vrykulAI : public SmartAI
        {
            npc_frostwing_vrykulAI(Creature* creature) : SmartAI(creature)
            {
            }

            bool CanAIAttack(Unit const* target) const
            {
                // do not see targets inside Frostwing Halls when we are not there
                return (me->GetPositionY() > 2660.0f) == (target->GetPositionY() > 2660.0f) && SmartAI::CanAIAttack(target);
            }
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_frostwing_vrykulAI(creature);
        }
};

class npc_impaling_spear : public CreatureScript
{
    public:
        npc_impaling_spear() : CreatureScript("npc_impaling_spear") { }

        struct npc_impaling_spearAI : public CreatureAI
        {
            npc_impaling_spearAI(Creature* creature) : CreatureAI(creature)
            {
            }

            void Reset()
            {
                me->SetReactState(REACT_PASSIVE);
                _vehicleCheckTimer = 500;
            }

            void UpdateAI(uint32 const diff)
            {
                if (_vehicleCheckTimer <= diff)
                {
                    _vehicleCheckTimer = 500;
                    if (!me->GetVehicle())
                        me->DespawnOrUnsummon(100);
                }
                else
                    _vehicleCheckTimer -= diff;
            }

            uint32 _vehicleCheckTimer;
        };

        CreatureAI* GetAI(Creature* creature) const
        {
            return new npc_impaling_spearAI(creature);
        }
};

class spell_icc_stoneform : public SpellScriptLoader
{
    public:
        spell_icc_stoneform() : SpellScriptLoader("spell_icc_stoneform") { }

        class spell_icc_stoneform_AuraScript : public AuraScript
        {
            PrepareAuraScript(spell_icc_stoneform_AuraScript);

            void OnApply(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Creature* target = GetTarget()->ToCreature())
                {
                    target->SetReactState(REACT_PASSIVE);
                    target->SetFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_OOC_NOT_ATTACKABLE);
                    target->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_CUSTOM_SPELL_02);
                }
            }

            void OnRemove(AuraEffect const* /*aurEff*/, AuraEffectHandleModes /*mode*/)
            {
                if (Creature* target = GetTarget()->ToCreature())
                {
                    target->SetReactState(REACT_AGGRESSIVE);
                    target->RemoveFlag(UNIT_FIELD_FLAGS, UNIT_FLAG_NOT_SELECTABLE | UNIT_FLAG_OOC_NOT_ATTACKABLE);
                    target->SetUInt32Value(UNIT_NPC_EMOTESTATE, 0);
                }
            }

            void Register()
            {
                OnEffectApply += AuraEffectApplyFn(spell_icc_stoneform_AuraScript::OnApply, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
                OnEffectRemove += AuraEffectRemoveFn(spell_icc_stoneform_AuraScript::OnRemove, EFFECT_0, SPELL_AURA_DUMMY, AURA_EFFECT_HANDLE_REAL);
            }
        };

        AuraScript* GetAuraScript() const
        {
            return new spell_icc_stoneform_AuraScript();
        }
};

class spell_icc_sprit_alarm : public SpellScriptLoader
{
    public:
        spell_icc_sprit_alarm() : SpellScriptLoader("spell_icc_sprit_alarm") { }

        class spell_icc_sprit_alarm_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icc_sprit_alarm_SpellScript);

            void HandleEvent(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                uint32 trapId = 0;
                switch (GetSpellInfo()->EffectMiscValue[effIndex])
                {
                    case EVENT_AWAKEN_WARD_1:
                        trapId = GO_SPIRIT_ALARM_1;
                        break;
                    case EVENT_AWAKEN_WARD_2:
                        trapId = GO_SPIRIT_ALARM_2;
                        break;
                    case EVENT_AWAKEN_WARD_3:
                        trapId = GO_SPIRIT_ALARM_3;
                        break;
                    case EVENT_AWAKEN_WARD_4:
                        trapId = GO_SPIRIT_ALARM_4;
                        break;
                    default:
                        return;
                }

                if (GameObject* trap = GetCaster()->FindNearestGameObject(trapId, 5.0f))
                    trap->SetRespawnTime(trap->GetGOInfo()->trap.autoCloseTime);

                std::list<Creature*> wards;
                GetCaster()->GetCreatureListWithEntryInGrid(wards, NPC_DEATHBOUND_WARD, 150.0f);
                wards.sort(Trinity::ObjectDistanceOrderPred(GetCaster()));
                for (std::list<Creature*>::iterator itr = wards.begin(); itr != wards.end(); ++itr)
                {
                    if ((*itr)->isAlive() && (*itr)->HasAura(SPELL_STONEFORM))
                    {
                        (*itr)->RemoveAurasDueToSpell(SPELL_STONEFORM);
                        if (Unit* target = (*itr)->SelectNearestTarget(150.0f))
                            (*itr)->AI()->AttackStart(target);
                        break;
                    }
                }
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_icc_sprit_alarm_SpellScript::HandleEvent, EFFECT_2, SPELL_EFFECT_SEND_EVENT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icc_sprit_alarm_SpellScript();
        }
};

class DeathPlagueTargetSelector
{
    public:
        explicit DeathPlagueTargetSelector(Unit* caster) : _caster(caster) {}

        bool operator()(Unit* unit)
        {
            if (unit == _caster)
                return true;

            if (unit->GetTypeId() != TYPEID_PLAYER)
                return true;

            if (unit->HasAura(SPELL_RECENTLY_INFECTED) || unit->HasAura(SPELL_DEATH_PLAGUE_AURA))
                return true;

            return false;
        }

    private:
        Unit* _caster;
};

class spell_frost_giant_death_plague : public SpellScriptLoader
{
    public:
        spell_frost_giant_death_plague() : SpellScriptLoader("spell_frost_giant_death_plague") { }

        class spell_frost_giant_death_plague_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_frost_giant_death_plague_SpellScript);

            bool Load()
            {
                _failed = false;
                return true;
            }

            // First effect
            void CountTargets(std::list<Unit*>& unitList)
            {
                unitList.remove(GetCaster());
                _failed = unitList.empty();
            }

            // Second effect
            void FilterTargets(std::list<Unit*>& unitList)
            {
                // Select valid targets for jump
                unitList.remove_if(DeathPlagueTargetSelector(GetCaster()));
                if (!unitList.empty())
                {
                    std::list<Unit*>::iterator itr = unitList.begin();
                    std::advance(itr, urand(0, unitList.size()-1));
                    Unit* target = *itr;
                    unitList.clear();
                    unitList.push_back(target);
                }

                unitList.push_back(GetCaster());
            }

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (GetHitUnit() != GetCaster())
                    GetCaster()->CastSpell(GetHitUnit(), SPELL_DEATH_PLAGUE_AURA, true);
                else if (_failed)
                    GetCaster()->CastSpell(GetCaster(), SPELL_DEATH_PLAGUE_KILL, true);
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_frost_giant_death_plague_SpellScript::CountTargets, EFFECT_0, TARGET_UNIT_AREA_ALLY_SRC);
                OnUnitTargetSelect += SpellUnitTargetFn(spell_frost_giant_death_plague_SpellScript::FilterTargets, EFFECT_1, TARGET_UNIT_AREA_ALLY_SRC);
                OnEffect += SpellEffectFn(spell_frost_giant_death_plague_SpellScript::HandleScript, EFFECT_1, SPELL_EFFECT_SCRIPT_EFFECT);
            }

            bool _failed;
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_frost_giant_death_plague_SpellScript();
        }
};

class spell_icc_harvest_blight_specimen : public SpellScriptLoader
{
    public:
        spell_icc_harvest_blight_specimen() : SpellScriptLoader("spell_icc_harvest_blight_specimen") { }

        class spell_icc_harvest_blight_specimen_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_icc_harvest_blight_specimen_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                GetHitUnit()->RemoveAurasDueToSpell(uint32(GetEffectValue()));
            }

            void HandleQuestComplete(SpellEffIndex /*effIndex*/)
            {
                GetHitUnit()->RemoveAurasDueToSpell(uint32(GetEffectValue()));
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_icc_harvest_blight_specimen_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
                OnEffect += SpellEffectFn(spell_icc_harvest_blight_specimen_SpellScript::HandleQuestComplete, EFFECT_1, SPELL_EFFECT_QUEST_COMPLETE);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_icc_harvest_blight_specimen_SpellScript();
        }
};

class AliveCheck
{
    public:
        bool operator()(Unit* unit)
        {
            return unit->isAlive();
        }
};

class spell_svalna_revive_champion : public SpellScriptLoader
{
    public:
        spell_svalna_revive_champion() : SpellScriptLoader("spell_svalna_revive_champion") { }

        class spell_svalna_revive_champion_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_svalna_revive_champion_SpellScript);

            void RemoveAliveTarget(std::list<Unit*>& unitList)
            {
                unitList.remove_if(AliveCheck());
                Trinity::RandomResizeList(unitList, 2);
            }

            void Register()
            {
                OnUnitTargetSelect += SpellUnitTargetFn(spell_svalna_revive_champion_SpellScript::RemoveAliveTarget, EFFECT_0, TARGET_UNIT_AREA_ENTRY_DST);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_svalna_revive_champion_SpellScript();
        }
};

class spell_svalna_remove_spear : public SpellScriptLoader
{
    public:
        spell_svalna_remove_spear() : SpellScriptLoader("spell_svalna_remove_spear") { }

        class spell_svalna_remove_spear_SpellScript : public SpellScript
        {
            PrepareSpellScript(spell_svalna_remove_spear_SpellScript);

            void HandleScript(SpellEffIndex effIndex)
            {
                PreventHitDefaultEffect(effIndex);
                if (Creature* target = GetHitCreature())
                    target->DespawnOrUnsummon();
            }

            void Register()
            {
                OnEffect += SpellEffectFn(spell_svalna_remove_spear_SpellScript::HandleScript, EFFECT_0, SPELL_EFFECT_SCRIPT_EFFECT);
            }
        };

        SpellScript* GetSpellScript() const
        {
            return new spell_svalna_remove_spear_SpellScript();
        }
};

class at_icc_saurfang_portal : public AreaTriggerScript
{
    public:
        at_icc_saurfang_portal() : AreaTriggerScript("at_icc_saurfang_portal") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            InstanceScript* instance = player->GetInstanceScript();
            if (!instance || instance->GetBossState(DATA_DEATHBRINGER_SAURFANG) != DONE)
                return true;

            player->TeleportTo(631, 4126.35f, 2769.23f, 350.963f, 0.0f);

            if (instance->GetData(DATA_COLDFLAME_JETS) == NOT_STARTED)
            {
                // Process relocation now, to preload the grid and initialize traps
                player->GetMap()->PlayerRelocation(player, 4126.35f, 2769.23f, 350.963f, 0.0f);

                instance->SetData(DATA_COLDFLAME_JETS, IN_PROGRESS);
                std::list<Creature*> traps;
                GetCreatureListWithEntryInGrid(traps, player, NPC_FROST_FREEZE_TRAP, 120.0f);
                traps.sort(Trinity::ObjectDistanceOrderPred(player));
                bool instant = false;
                for (std::list<Creature*>::iterator itr = traps.begin(); itr != traps.end(); ++itr)
                {
                    (*itr)->AI()->DoAction(instant ? 1000 : 11000);
                    instant = !instant;
                }
            }

>>>>>>> 84ab669... Scripts/Icecrown Citadel: Fixed Deathbound wards & their trap linking
            return true;
        }
};

class at_icc_shutdown_traps : public AreaTriggerScript
{
    public:
        at_icc_shutdown_traps() : AreaTriggerScript("at_icc_shutdown_traps") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_COLDFLAME_JETS) == IN_PROGRESS)
                    instance->SetData(DATA_COLDFLAME_JETS, DONE);
            return true;
        }
};

class at_icc_start_blood_quickening : public AreaTriggerScript
{
    public:
        at_icc_start_blood_quickening() : AreaTriggerScript("at_icc_start_blood_quickening") { }

        bool OnTrigger(Player* player, AreaTriggerEntry const* /*areaTrigger*/)
        {
            if (InstanceScript* instance = player->GetInstanceScript())
                if (instance->GetData(DATA_BLOOD_QUICKENING_STATE) == NOT_STARTED)
                    instance->SetData(DATA_BLOOD_QUICKENING_STATE, IN_PROGRESS);
            return true;
        }
};

void AddSC_icecrown_citadel()
{
    new npc_highlord_tirion_fordring_lh();
    new npc_rotting_frost_giant();
    new npc_frost_freeze_trap();
    new npc_alchemist_adrianna();
    new boss_sister_svalna();
    new npc_crok_scourgebane();
    new npc_captain_arnath();
    new npc_captain_brandon();
    new npc_captain_grondel();
    new npc_captain_rupert();
    new npc_frostwing_vrykul();
    new npc_impaling_spear();
    new spell_icc_stoneform();
    new spell_icc_sprit_alarm();
    new spell_frost_giant_death_plague();
    new spell_icc_harvest_blight_specimen();
    new at_icc_saurfang_portal();
    new at_icc_shutdown_traps();
    new at_icc_start_blood_quickening();
}
