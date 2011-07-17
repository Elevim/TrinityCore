/*
 * Copyright (C) 2008-2011 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2006-2009 ScriptDev2 <https://scriptdev2.svn.sourceforge.net/>
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

/* ScriptData
SDName: Stranglethorn_Vale
SD%Complete: 100
SDComment: Quest support: 592
SDCategory: Stranglethorn Vale
EndScriptData */

/* ContentData
mob_yenniku
EndContentData */

#include "ScriptPCH.h"

/*######
## mob_yenniku
######*/

class mob_yenniku : public CreatureScript
{
public:
    mob_yenniku() : CreatureScript("mob_yenniku") { }

    CreatureAI* GetAI(Creature* creature) const
    {
        return new mob_yennikuAI (creature);
    }

    struct mob_yennikuAI : public ScriptedAI
    {
        mob_yennikuAI(Creature* c) : ScriptedAI(c)
        {
            bReset = false;
        }

        uint32 Reset_Timer;
        bool bReset;

        void Reset()
        {
            Reset_Timer = 0;
            me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_NONE);
        }

        void SpellHit(Unit* caster, const SpellEntry *spell)
        {
            if (caster->GetTypeId() == TYPEID_PLAYER)
            {
                                                                //Yenniku's Release
                if (!bReset && CAST_PLR(caster)->GetQuestStatus(592) == QUEST_STATUS_INCOMPLETE && spell->Id == 3607)
                {
                    me->SetUInt32Value(UNIT_NPC_EMOTESTATE, EMOTE_STATE_STUN);
                    me->CombatStop();                   //stop combat
                    me->DeleteThreatList();             //unsure of this
                    me->setFaction(83);                 //horde generic

                    bReset = true;
                    Reset_Timer = 60000;
                }
            }
            return;
        }

        void EnterCombat(Unit* /*who*/) {}

        void UpdateAI(const uint32 diff)
        {
            if (bReset)
            {
                if (Reset_Timer <= diff)
                {
                    EnterEvadeMode();
                    bReset = false;
                    me->setFaction(28);                     //troll, bloodscalp
                    return;
                }
                else Reset_Timer -= diff;

                if (me->isInCombat() && me->getVictim())
                {
                    if (me->getVictim()->GetTypeId() == TYPEID_PLAYER)
                    {
                        Unit* victim = me->getVictim();
                        if (CAST_PLR(victim)->GetTeam() == HORDE)
                        {
                            me->CombatStop();
                            me->DeleteThreatList();
                        }
                    }
                }
             }

            //Return since we have no target
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
        }
    };

};

enum Says
{
    SAY_START               = -1510356,
    SAY_WINNER              = -1510357,
    SAY_END                 = -1510358,
    QUEST_MASTER_ANGLER     = 8193,
};
/*######
## npc_riggle_bassbait
######*/
class npc_riggle_bassbait : public CreatureScript
{
public:
    npc_riggle_bassbait() : CreatureScript("npc_riggle_bassbait") { }

    bool OnGossipHello(Player* pPlayer, Creature* pCreature)
    {
        if (pCreature->isQuestGiver()) // If the quest is still running.
        {
            pPlayer->PrepareQuestMenu(pCreature->GetGUID());
            pPlayer->SEND_GOSSIP_MENU(7614, pCreature->GetGUID());
            return true;
        }
        // The Quest is not there anymore
        // There is a winner!
        pPlayer->SEND_GOSSIP_MENU(7714, pCreature->GetGUID());
        return true;
    }

    bool OnQuestReward(Player* pPlayer, Creature* pCreature, const Quest* pQuest, uint32 opt)
    {
        // TODO: check if this can only be called if NPC has QUESTGIVER flag.
        if (pQuest->GetQuestId() == QUEST_MASTER_ANGLER && ((npc_riggle_bassbaitAI*)(pCreature->AI()))->bEventWinnerFound == false)
        {
            DoScriptText(SAY_WINNER, pCreature,pPlayer);
            pCreature->RemoveFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER);
            ((npc_riggle_bassbaitAI*)(pCreature->AI()))->bEventWinnerFound = true;
            Creature* creature2 = GetClosestCreatureWithEntry(pCreature,15087,60.0f);
            if (creature2)
            {
                creature2->SetFlag(UNIT_NPC_FLAGS,UNIT_NPC_FLAG_QUESTGIVER);
            }

            return true;
        }
    return true;
}


    CreatureAI* GetAI(Creature* creature) const
    {
        return new npc_riggle_bassbaitAI (creature);
    }

    struct npc_riggle_bassbaitAI: public ScriptedAI
    {
        npc_riggle_bassbaitAI(Creature* c) : ScriptedAI(c)
        {
            // This will keep the NPC active even if there are no players around!
            c->setActive(true);
            bEventAnnounced = bEventIsOver = bEventWinnerFound = false;
            Reset();
        }
        /**
        *  Flag to check if event was announced. True if event was announced.
        */
        bool bEventAnnounced;
        /**
         *  Flag to check if event is over. True if event is over.
         */
        bool bEventIsOver;
        /**
         *  Flag to check if someone won the event. True if someone has won.
         */
        bool bEventWinnerFound;

        void Reset() { }
        void EnterCombat(Unit* who) {}

        void UpdateAI(uint32 uiDiff)
        {
            // Announce the event max 1 minute after being spawned. But only if Fishing extravaganza is running.
            if (!bEventAnnounced && time(NULL) % 60 == 0 && IsHolidayActive(HOLIDAY_FISHING_EXTRAVAGANZA))
            {
                DoScriptText(SAY_START, me);
                me->SetFlag(UNIT_NPC_FLAGS, UNIT_NPC_FLAG_QUESTGIVER); //Quest&Gossip are now active
                bEventAnnounced = true;
            }
            // The Event was started (announced) & It was not yet ended & One minute passed & the Fish are gone
            if ( bEventAnnounced && !bEventIsOver && time(NULL) % 60 == 0 && !IsHolidayActive(HOLIDAY_FISHING_EXTRAVAGANZA))
            {
                DoScriptText(SAY_END, me);
                bEventIsOver = true;

            }
            if (!UpdateVictim())
                return;

            DoMeleeAttackIfReady();
    }
    };
};

/*######
##
######*/

void AddSC_stranglethorn_vale()
{
    new mob_yenniku();
    new npc_riggle_bassbait();
}
