/*
5.0
Transmogrification 3.3.5a - Gossip menu
By Rochet2

ScriptName for player:
tlk_player_transmog

Adapted to PlayerScript by Ayahuasca-Ruderalis for TheLegendaryKingdom
*/
#include "Define.h"
#include "GossipDef.h"
#include "Item.h"
#include "Player.h"
#include "ScriptedGossip.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "Configuration/Config.h"
#include "Chat.h"
#include "Transmogrification.h"
#define sT  sTransmogrification
#define GTS session->GetAcoreString // dropped translation support, no one using?

class player_transmog : public PlayerScript
{
public:
    player_transmog() : PlayerScript("tlk_player_transmog") { }

    void OnSpellCast(Player* player, Spell* spell, bool /*skipCheck*/) override
    {
        if (spell->GetSpellInfo()->Id == 81102)
            ShowGossipHello(player);
        
        return;
    }

    bool ShowGossipHello(Player* player)
    {
        CloseGossipMenuFor(player);
        ClearGossipMenuFor(player);
        player->PlayerTalkClass->ClearMenus();        
        
        WorldSession* session = player->GetSession();
        if (sT->GetEnableTransmogInfo())
            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "|TInterface/ICONS/INV_Misc_Book_11:20:20:0:0|t Informations sur la transmogrification", EQUIPMENT_SLOT_END + 9, 0);
        for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
        {
            if (const char* slotName = sT->GetSlotName(slot, session))
            {
                Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
                uint32 entry = newItem ? sT->GetFakeEntry(newItem->GetGUID()) : 0;
                std::string icon = entry ? sT->GetItemIcon(entry, 20, 20, 0, 0) : sT->GetSlotIcon(slot, 20, 20, 0, 0);
                AddGossipItemFor(player, GOSSIP_ICON_VENDOR, icon + std::string(slotName), EQUIPMENT_SLOT_END, slot);
            }
        }
#ifdef PRESETS
        if (sT->GetEnableSets())
            AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/ICONS/INV_Misc_Statue_02:20:20:0:0|t Gérer les ensembles", EQUIPMENT_SLOT_END + 4, 0);
#endif
        AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface/ICONS/INV_Enchant_Disenchant:20:20:0:0|t Retirer toutes les transmogrifications", EQUIPMENT_SLOT_END + 2, 0, "Voulez-vous vraiment retirer les transmogrifications de TOUS les objets équipés ?", 0, false);
        AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:20:20:0:0|t Rafraîchir", EQUIPMENT_SLOT_END + 1, 0);
        SendGossipMenuFor(player, 60004, player->GetGUID());
        return true;        
    }

    void OnGossipSelect(Player* player, uint32 /*menu_id*/, uint32 sender, uint32 action) override
    {
        if (action >= 1000000)
            return;
        
        player->PlayerTalkClass->ClearMenus();
        WorldSession* session = player->GetSession();
        // Next page
        if (sender > EQUIPMENT_SLOT_END + 10)
        {
            ShowTransmogItems(player, action, sender);
            return;
        }
        switch (sender)
        {
            case EQUIPMENT_SLOT_END: // Show items you can use
                ShowTransmogItems(player, action, sender);
                break;
            case EQUIPMENT_SLOT_END + 1: // Main menu
                ShowGossipHello(player);
                break;
            case EQUIPMENT_SLOT_END + 2: // Remove Transmogrifications
            {
                bool removed = false;
                auto trans = CharacterDatabase.BeginTransaction();
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        if (!sT->GetFakeEntry(newItem->GetGUID()))
                            continue;
                        sT->DeleteFakeEntry(player, slot, newItem, &trans);
                        removed = true;
                    }
                }
                if (removed)
                {
                    session->SendAreaTriggerMessage("%s", GTS(LANG_ERR_UNTRANSMOG_OK));
                    CharacterDatabase.CommitTransaction(trans);
                }
                else
                    session->SendNotification(LANG_ERR_UNTRANSMOG_NO_TRANSMOGS);
                ShowGossipHello(player);
            } break;
            case EQUIPMENT_SLOT_END + 3: // Remove Transmogrification from single item
            {
                if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, action))
                {
                    if (sT->GetFakeEntry(newItem->GetGUID()))
                    {
                        sT->DeleteFakeEntry(player, action, newItem);
                        session->SendAreaTriggerMessage("%s", GTS(LANG_ERR_UNTRANSMOG_OK));
                    }
                    else
                        session->SendNotification(LANG_ERR_UNTRANSMOG_NO_TRANSMOGS);
                }
                OnGossipSelect(player, 0, EQUIPMENT_SLOT_END, action);
            } break;
    #ifdef PRESETS
            case EQUIPMENT_SLOT_END + 4: // Presets menu
            {
                if (!sT->GetEnableSets())
                {
                    ShowGossipHello(player);
                    return;
                }
                if (sT->GetEnableSetInfo())
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "|TInterface/ICONS/INV_Misc_Book_11:20:20|t Informations sur les ensembles", EQUIPMENT_SLOT_END + 10, 0);
                for (Transmogrification::presetIdMap::const_iterator it = sT->presetByName[player->GetGUID()].begin(); it != sT->presetByName[player->GetGUID()].end(); ++it)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, "|TInterface/ICONS/INV_Misc_Statue_02:20:20|t" + it->second, EQUIPMENT_SLOT_END + 6, it->first);

                if (sT->presetByName[player->GetGUID()].size() < sT->GetMaxSets())
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/GuildBankFrame/UI-GuildBankFrame-NewTab:20:20|t Enregistrer un ensemble", EQUIPMENT_SLOT_END + 8, 0);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Ability_Spy:20:20|t Retour", EQUIPMENT_SLOT_END + 1, 0);
                SendGossipMenuFor(player, 60004, player->GetGUID());
            } break;
            case EQUIPMENT_SLOT_END + 5: // Use preset
            {
                if (!sT->GetEnableSets())
                {
                    ShowGossipHello(player);
                    return;
                }
                // action = presetID
                for (Transmogrification::slotMap::const_iterator it = sT->presetById[player->GetGUID()][action].begin(); it != sT->presetById[player->GetGUID()][action].end(); ++it)
                {
                    if (Item* item = player->GetItemByPos(INVENTORY_SLOT_BAG_0, it->first))
                        sT->PresetTransmog(player, item, it->second, it->first);
                }
                OnGossipSelect(player, 0, EQUIPMENT_SLOT_END + 6, action);
            } break;
            case EQUIPMENT_SLOT_END + 6: // view preset
            {
                if (!sT->GetEnableSets())
                {
                    ShowGossipHello(player);
                    return;
                }
                // action = presetID
                for (Transmogrification::slotMap::const_iterator it = sT->presetById[player->GetGUID()][action].begin(); it != sT->presetById[player->GetGUID()][action].end(); ++it)
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, sT->GetItemIcon(it->second, 20, 20, 0, 0) + sT->GetItemLink(it->second, session), sender, action);

                AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/ICONS/INV_Misc_Statue_02:20:20|t Utiliser cet ensemble", EQUIPMENT_SLOT_END + 5, action, "Appliquer l'ensemble de transmogrification \"" + sT->presetByName[player->GetGUID()][action] + "\" ?", 0, false);
                AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface/PaperDollInfoFrame/UI-GearManager-LeaveItem-Opaque:20:20|t Supprimer l'ensemble", EQUIPMENT_SLOT_END + 7, action, "Voulez-vous vraiment supprimer l'ensemble de transmogrification \"" + sT->presetByName[player->GetGUID()][action] + "\" ?", 0, false);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Ability_Spy:20:20|t Retour", EQUIPMENT_SLOT_END + 4, 0);
                SendGossipMenuFor(player, 60004, player->GetGUID());
            } break;
            case EQUIPMENT_SLOT_END + 7: // Delete preset
            {
                if (!sT->GetEnableSets())
                {
                    ShowGossipHello(player);
                    return;
                }
                // action = presetID
                CharacterDatabase.Execute("DELETE FROM `custom_transmogrification_sets` WHERE Owner = {} AND PresetID = {}", player->GetGUID().GetCounter(), action);
                sT->presetById[player->GetGUID()][action].clear();
                sT->presetById[player->GetGUID()].erase(action);
                sT->presetByName[player->GetGUID()].erase(action);

                OnGossipSelect(player, 0, EQUIPMENT_SLOT_END + 4, 0);
            } break;
            case EQUIPMENT_SLOT_END + 8: // Save preset
            {
                if (!sT->GetEnableSets() || sT->presetByName[player->GetGUID()].size() >= sT->GetMaxSets())
                {
                    ShowGossipHello(player);
                    return;
                }
                uint32 cost = 0;
                bool canSave = false;
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (!sT->GetSlotName(slot, session))
                        continue;
                    if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        uint32 entry = sT->GetFakeEntry(newItem->GetGUID());
                        if (!entry)
                            continue;
                        const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
                        if (!temp)
                            continue;
                        if (!sT->SuitableForTransmogrification(player, temp)) // no need to check?
                            continue;
                        cost += sT->GetSpecialPrice(temp);
                        canSave = true;
                        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, sT->GetItemIcon(entry, 20, 20, 0, 0) + sT->GetItemLink(entry, session), EQUIPMENT_SLOT_END + 8, 0);
                    }
                }
                if (canSave)
                    AddGossipItemFor(player, GOSSIP_ICON_INTERACT_1, "|TInterface/GuildBankFrame/UI-GuildBankFrame-NewTab:20:20|t Enregistrer l'ensemble", 0, 0, "Pour enregistrer cet ensemble, vous devrez lui attribuer un code identifiant (nom) dans la prochaine boîte de dialogue.\n\nVoulez-vous continuer ?", cost*sT->GetSetCostModifier() + sT->GetSetCopperCost(), true);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:20:20|t Rafraîchir", sender, action);
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Ability_Spy:20:20|t Retour", EQUIPMENT_SLOT_END + 4, 0);
                SendGossipMenuFor(player, 60004, player->GetGUID());
            } break;
            case EQUIPMENT_SLOT_END + 10: // Set info
            {
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Ability_Spy:20:20|t Retour", EQUIPMENT_SLOT_END + 4, 0);
                SendGossipMenuFor(player, sT->GetSetNpcText(), player->GetGUID());
            } break;
    #endif
            case EQUIPMENT_SLOT_END + 9: // Transmog info
            {
                AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Ability_Spy:20:20|t Retour", EQUIPMENT_SLOT_END + 1, 0);
                SendGossipMenuFor(player, sT->GetTransmogNpcText(), player->GetGUID());
            } break;
            default: // Transmogrify
            {
                if (!sender && !action)
                {
                    ShowGossipHello(player);
                    return;
                }
                // sender = slot, action = display
                if (sT->GetUseCollectionSystem())
                {
                    TransmogAcoreStrings res = sT->Transmogrify(player, action, sender);
                    if (res == LANG_ERR_TRANSMOG_OK)
                        session->SendAreaTriggerMessage("%s",GTS(LANG_ERR_TRANSMOG_OK));
                    else
                        session->SendNotification(res);
                }
                else
                {
                    TransmogAcoreStrings res = sT->Transmogrify(player, ObjectGuid::Create<HighGuid::Item>(action), sender);
                    if (res == LANG_ERR_TRANSMOG_OK)
                        session->SendAreaTriggerMessage("%s",GTS(LANG_ERR_TRANSMOG_OK));
                    else
                        session->SendNotification(res);
                }
                OnGossipSelect(player, 0, EQUIPMENT_SLOT_END, sender);
            } break;
        }
        return;
        
    }

#ifdef PRESETS
    void OnGossipSelectCode(Player* player, uint32 /*menu_id*/, uint32 sender, uint32 action, const char* code) override
    {
        player->PlayerTalkClass->ClearMenus();
        if (sender || action)
            return; // should never happen
        if (!sT->GetEnableSets())
        {
            ShowGossipHello(player);
            return;
        }
        std::string name(code);
        if (name.find('"') != std::string::npos || name.find('\\') != std::string::npos)
            player->GetSession()->SendNotification(LANG_PRESET_ERR_INVALID_NAME);
        else
        {
            for (uint8 presetID = 0; presetID < sT->GetMaxSets(); ++presetID) // should never reach over max
            {
                if (sT->presetByName[player->GetGUID()].find(presetID) != sT->presetByName[player->GetGUID()].end())
                    continue; // Just remember never to use presetByName[pGUID][presetID] when finding etc!

                int32 cost = 0;
                std::map<uint8, uint32> items;
                for (uint8 slot = EQUIPMENT_SLOT_START; slot < EQUIPMENT_SLOT_END; ++slot)
                {
                    if (!sT->GetSlotName(slot, player->GetSession()))
                        continue;
                    if (Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot))
                    {
                        uint32 entry = sT->GetFakeEntry(newItem->GetGUID());
                        if (!entry)
                            continue;
                        const ItemTemplate* temp = sObjectMgr->GetItemTemplate(entry);
                        if (!temp)
                            continue;
                        if (!sT->SuitableForTransmogrification(player, temp))
                            continue;
                        cost += sT->GetSpecialPrice(temp);
                        items[slot] = entry;
                    }
                }
                if (items.empty())
                    break; // no transmogrified items were found to be saved
                cost *= sT->GetSetCostModifier();
                cost += sT->GetSetCopperCost();
                if (!player->HasEnoughMoney(cost))
                {
                    player->GetSession()->SendNotification(LANG_ERR_TRANSMOG_NOT_ENOUGH_MONEY);
                    break;
                }

                std::ostringstream ss;
                for (std::map<uint8, uint32>::iterator it = items.begin(); it != items.end(); ++it)
                {
                    ss << uint32(it->first) << ' ' << it->second << ' ';
                    sT->presetById[player->GetGUID()][presetID][it->first] = it->second;
                }
                sT->presetByName[player->GetGUID()][presetID] = name; // Make sure code doesnt mess up SQL!
                CharacterDatabase.Execute("REPLACE INTO `custom_transmogrification_sets` (`Owner`, `PresetID`, `SetName`, `SetData`) VALUES ({}, {}, \"{}\", \"{}\")", player->GetGUID().GetCounter(), uint32(presetID), name, ss.str());
                if (cost)
                    player->ModifyMoney(-cost);
                break;
            }
        }
        ShowGossipHello(player);
        return;
        
        
    }
#endif

    void ShowTransmogItems(Player* player, uint8 slot, uint16 gossipPageNumber) // Only checks bags while can use an item from anywhere in inventory
    {
        WorldSession* session = player->GetSession();
        Item* oldItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, slot);
        bool sendGossip = true;
        if (oldItem)
        {
            uint32 price = sT->GetSpecialPrice(oldItem->GetTemplate());
            price *= sT->GetScaledCostModifier();
            price += sT->GetCopperCost();
            std::ostringstream ss;
            ss << std::endl;
            if (sT->GetRequireToken())
                ss << std::endl << std::endl << sT->GetTokenAmount() << " x " << sT->GetItemLink(sT->GetTokenEntry(), session);
            std::string lineEnd = ss.str();

            if (sT->GetUseCollectionSystem())
            {
                sendGossip = false;

                std::string query = "SELECT item_template_id FROM custom_unlocked_appearances WHERE account_id = 0 OR account_id = " + std::to_string(player->GetSession()->GetAccountId()) + " ORDER BY item_template_id";
                session->GetQueryProcessor().AddCallback(LoginDatabase.AsyncQuery(query).WithCallback([=](QueryResult result)
                {
                    uint16 pageNumber = 0;
                    uint32 startValue = 0;
                    uint32 endValue = MAX_OPTIONS - 3;
                    bool lastPage = false;
                    if (gossipPageNumber > EQUIPMENT_SLOT_END + 10)
                    {
                        pageNumber = gossipPageNumber - EQUIPMENT_SLOT_END - 10;
                        startValue = (pageNumber * (MAX_OPTIONS - 2));
                        endValue = (pageNumber + 1) * (MAX_OPTIONS - 2) - 1;
                    }
                    if (result)
                    {
                        std::vector<Item*> allowedItems;
                        do {
                            uint32 newItemEntryId = (*result)[0].Get<uint32>();
                            Item* newItem = Item::CreateItem(newItemEntryId, 1, 0);
                            if (!newItem)
                                continue;
                            if (!sT->CanTransmogrifyItemWithItem(player, oldItem->GetTemplate(), newItem->GetTemplate()))
                                continue;
                            if (sT->GetFakeEntry(oldItem->GetGUID()) == newItem->GetEntry())
                                continue;
                            allowedItems.push_back(newItem);
                        } while (result->NextRow());
                        for (uint32 i = startValue; i <= endValue; i++)
                        {
                            if (allowedItems.empty() || i > allowedItems.size() - 1)
                            {
                                lastPage = true;
                                break;
                            }
                            Item* newItem = allowedItems.at(i);
                            AddGossipItemFor(player, GOSSIP_ICON_VENDOR, sT->GetItemIcon(newItem->GetEntry(), 20, 20, 0, 0) + sT->GetItemLink(newItem, session), slot, newItem->GetEntry(), "Utiliser l'apparence de cet objet ?\n\n" + sT->GetItemIcon(newItem->GetEntry(), 40, 40, -15, -10) + sT->GetItemLink(newItem, session) + lineEnd, price, false);
                        }
                    }
                    if (gossipPageNumber == EQUIPMENT_SLOT_END + 11)
                    {
                        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Page précédente", EQUIPMENT_SLOT_END, slot);
                        if (!lastPage)
                        {
                            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Page suivante", gossipPageNumber + 1, slot);
                        }
                    }
                    else if (gossipPageNumber > EQUIPMENT_SLOT_END + 11)
                    {
                        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Page précédente", gossipPageNumber - 1, slot);
                        if (!lastPage)
                        {
                            AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Page suivante", gossipPageNumber + 1, slot);
                        }
                    }
                    else if (!lastPage)
                    {
                        AddGossipItemFor(player, GOSSIP_ICON_CHAT, "Page suivante", EQUIPMENT_SLOT_END + 11, slot);
                    }

                    AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface/ICONS/INV_Enchant_Disenchant:20:20|t Retirer la transmogrification", EQUIPMENT_SLOT_END + 3, slot, "Voulez-vous vraiment retirer cette transmogrification ?", 0, false);
                    AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:20:20|t Rafraîchir", EQUIPMENT_SLOT_END, slot);
                    AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Ability_Spy:20:20|t Retour", EQUIPMENT_SLOT_END + 1, 0);
                    SendGossipMenuFor(player, 60004, player->GetGUID());
                }));
            }
            else
            {
                uint32 limit = 0;
                for (uint8 i = INVENTORY_SLOT_ITEM_START; i < INVENTORY_SLOT_ITEM_END; ++i)
                {
                    if (limit > MAX_OPTIONS)
                        break;
                    Item* newItem = player->GetItemByPos(INVENTORY_SLOT_BAG_0, i);
                    if (!newItem)
                        continue;
                    if (!sT->CanTransmogrifyItemWithItem(player, oldItem->GetTemplate(), newItem->GetTemplate()))
                        continue;
                    if (sT->GetFakeEntry(oldItem->GetGUID()) == newItem->GetEntry())
                        continue;
                    ++limit;
                    AddGossipItemFor(player, GOSSIP_ICON_VENDOR, sT->GetItemIcon(newItem->GetEntry(), 20, 20, 0, 0) + sT->GetItemLink(newItem, session), slot, newItem->GetGUID().GetCounter(), "Utiliser l'apparence de cet objet ?\n\n" + sT->GetItemIcon(newItem->GetEntry(), 40, 40, -15, -10) + sT->GetItemLink(newItem, session) + lineEnd, price, false);
                }

                for (uint8 i = INVENTORY_SLOT_BAG_START; i < INVENTORY_SLOT_BAG_END; ++i)
                {
                    Bag* bag = player->GetBagByPos(i);
                    if (!bag)
                        continue;
                    for (uint32 j = 0; j < bag->GetBagSize(); ++j)
                    {
                        if (limit > MAX_OPTIONS)
                            break;
                        Item* newItem = player->GetItemByPos(i, j);
                        if (!newItem)
                            continue;
                        if (!sT->CanTransmogrifyItemWithItem(player, oldItem->GetTemplate(), newItem->GetTemplate()))
                            continue;
                        if (sT->GetFakeEntry(oldItem->GetGUID()) == newItem->GetEntry())
                            continue;
                        ++limit;
                        AddGossipItemFor(player, GOSSIP_ICON_VENDOR, sT->GetItemIcon(newItem->GetEntry(), 20, 20, 0, 0) + sT->GetItemLink(newItem, session), slot, newItem->GetGUID().GetCounter(), "Utiliser l'apparence de cet objet ?\n\n" + sT->GetItemIcon(newItem->GetEntry(), 40, 40, -15, -10) + sT->GetItemLink(newItem, session) + ss.str(), price, false);
                    }
                }
            }
        }

        if (sendGossip) {
            AddGossipItemFor(player, GOSSIP_ICON_BATTLE, "|TInterface/ICONS/INV_Enchant_Disenchant:20:20|t Retirer la transmogrification", EQUIPMENT_SLOT_END + 3, slot, " Voulez-vous vraiment retirer cette transmogrification ?", 0, false);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/PaperDollInfoFrame/UI-GearManager-Undo:20:20|t Rafraîchir", EQUIPMENT_SLOT_END, slot);
            AddGossipItemFor(player, GOSSIP_ICON_TALK, "|TInterface/ICONS/Ability_Spy:20:20|t Retour", EQUIPMENT_SLOT_END + 1, 0);
            SendGossipMenuFor(player, 60004, player->GetGUID());
        }
        
    }
};



void AddSC_Transmog_Player()
{
    new player_transmog();
}

