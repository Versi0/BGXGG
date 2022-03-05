#include "../plugin_sdk/plugin_sdk.hpp"
#include "veigar.h"

namespace veigar
{

    #define Q_DRAW_COLOR (MAKE_COLOR ( 62, 129, 237, 255 ))
    #define W_DRAW_COLOR (MAKE_COLOR ( 227, 203, 20, 255 ))
    #define E_DRAW_COLOR (MAKE_COLOR ( 235, 12, 223, 255 ))
    #define R_DRAW_COLOR (MAKE_COLOR ( 224, 77, 13, 255 ))
    #define FLASH_DRAW_COLOR (MAKE_COLOR ( 0, 255, 23, 255 ))

    script_spell* q = nullptr;
    script_spell* w = nullptr;
    script_spell* e = nullptr;
    script_spell* r = nullptr;

    script_spell* flash = nullptr;

    TreeTab* main_tab = nullptr;

    namespace combo
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
        TreeEntry* w_auto_on_stun = nullptr;
        TreeEntry* w_auto_dashing = nullptr;
        TreeEntry* use_e = nullptr;
        TreeEntry* e_only_if_w_ready = nullptr;
        TreeEntry* use_r = nullptr;
        TreeEntry* r_flash_above_r_range = nullptr;
        std::map<std::uint32_t, TreeEntry*> r_use_on;
    }

    namespace harass
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
    }

    namespace lasthit
    {
        TreeEntry* use_q = nullptr;
    }

    namespace laneclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
    }

    namespace jungleclear
    {
        TreeEntry* use_q = nullptr;
        TreeEntry* use_w = nullptr;
    }

    namespace flee
    {
        TreeEntry* use_e;
    }

    namespace hitchance
    {
        TreeEntry* q_hitchance = nullptr;
        TreeEntry* w_hitchance = nullptr;
        TreeEntry* e_hitchance = nullptr;
    }

    namespace draw_settings
    {
        TreeEntry* draw_range_q = nullptr;
        TreeEntry* draw_range_w = nullptr;
        TreeEntry* draw_range_e = nullptr;
        TreeEntry* draw_range_r = nullptr;
        TreeEntry* draw_flash_range = nullptr;
    }

    void on_update();
    void on_draw();

    inline void draw_dmg_rl(game_object_script target, float damage, unsigned long color);
    bool can_use_r_on(game_object_script target);
    hit_chance get_hitchance(TreeEntry* entry);

    void q_logic();
    void w_logic();
    void e_logic();
    void r_logic();

    void load()
    {
        q = plugin_sdk->register_spell(spellslot::q, 950);
        q->set_skillshot(0.25f, 140.0f, 2200.0f, { collisionable_objects::yasuo_wall, collisionable_objects::heroes, collisionable_objects::minions }, skillshot_type::skillshot_line);
        w = plugin_sdk->register_spell(spellslot::w, 900);
        w->set_skillshot(0.25f, 240.0f, 0.0f, { }, skillshot_type::skillshot_circle);
        e = plugin_sdk->register_spell(spellslot::e, 725);
        e->set_skillshot(0.25f, 400.0f, 0.0f, { }, skillshot_type::skillshot_circle);
        r = plugin_sdk->register_spell(spellslot::r, 650);

        if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner1, 400.f);
        else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
            flash = plugin_sdk->register_spell(spellslot::summoner2, 400.f);

        main_tab = menu->create_tab("veigar", "Veigar");
        main_tab->set_assigned_texture(myhero->get_square_icon_portrait());
        {
            auto combo = main_tab->add_tab(myhero->get_model() + ".combo", "Combo Settings");
            {
                combo::use_q = combo->add_checkbox(myhero->get_model() + ".combo.q", "Use Q", true);
                combo::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                combo::use_w = combo->add_checkbox(myhero->get_model() + ".combo.w", "Use W", true);
                combo::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                auto w_config = combo->add_tab(myhero->get_model() + ".combo.w.config", "W Config");
                {
                    combo::w_auto_on_stun = w_config->add_checkbox(myhero->get_model() + ".combo.w.stun", "Auto W on stun", true);
                    combo::w_auto_dashing = w_config->add_checkbox(myhero->get_model() + ".combo.w.dashing", "Auto W dashing", true);
                }
                combo::use_e = combo->add_checkbox(myhero->get_model() + ".combo.e", "Use E", true);
                combo::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                auto e_config = combo->add_tab(myhero->get_model() + ".combo.e.config", "E Config");
                {
                    combo::e_only_if_w_ready = e_config->add_checkbox(myhero->get_model() + ".combo.e.only_w_ready", "Use E only if W is ready", true);
                }
                combo::use_r = combo->add_checkbox(myhero->get_model() + ".combo.r", "Use R", true);
                combo::use_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
                auto r_config = combo->add_tab(myhero->get_model() + ".combo.r.config", "R Config");
                {
                    combo::r_flash_above_r_range = r_config->add_checkbox(myhero->get_model() + ".combo.r.flash", "Use Flash + R above R range", true);
                    auto use_r_on_tab = r_config->add_tab(myhero->get_model() + ".combo.r.use_on", "Use R On");
                    {
                        for (auto&& enemy : entitylist->get_enemy_heroes())
                        {
                            combo::r_use_on[enemy->get_network_id()] = use_r_on_tab->add_checkbox(std::to_string(enemy->get_network_id()), enemy->get_model(), true, false);
                            combo::r_use_on[enemy->get_network_id()]->set_texture(enemy->get_square_icon_portrait());
                        }
                    }
                }
            }

            auto harass = main_tab->add_tab(myhero->get_model() + ".harass", "Harass Settings");
            {
                harass::use_q = harass->add_checkbox(myhero->get_model() + ".harass.q", "Use Q", true);
                harass::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                harass::use_w = harass->add_checkbox(myhero->get_model() + ".harass.w", "Use W", true);
                harass::use_w->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }

            auto lasthit = main_tab->add_tab(myhero->get_model() + ".lasthit", "Last Hit Settings");
            {
                lasthit::use_q = lasthit->add_checkbox(myhero->get_model() + ".lasthit.q", "Use Q", true);
                lasthit::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
            }

            auto laneclear = main_tab->add_tab(myhero->get_model() + ".laneclear", "Lane Clear Settings");
            {
                laneclear::use_q = laneclear->add_checkbox(myhero->get_model() + ".laneclear.q", "Use Q", false);
                laneclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                laneclear::use_w = laneclear->add_checkbox(myhero->get_model() + ".lanclear.w", "Use W", false);
                laneclear::use_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
            }

            auto jungleclear = main_tab->add_tab(myhero->get_model() + ".jungleclear", "Jungle Clear Settings");
            {
                jungleclear::use_q = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.q", "Use Q", true);
                jungleclear::use_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                jungleclear::use_w = jungleclear->add_checkbox(myhero->get_model() + ".jungleclear.w", "Use W", true);
                jungleclear::use_w->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }

            auto flee = main_tab->add_tab(myhero->get_model() + ".flee", "Flee");
            {
                flee::use_e = flee->add_checkbox(myhero->get_model() + ".flee.e", "Use E", true);
                flee::use_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
            }

            auto hitchance = main_tab->add_tab(myhero->get_model() + ".hitchance", "Hitchance Settings");
            {
                hitchance::q_hitchance = hitchance->add_combobox(myhero->get_model() + ".hitchance.q", "Hitchance Q", { {"Low",nullptr},{"Medium",nullptr },{"High", nullptr},{"Very High",nullptr} }, 2);
                hitchance::w_hitchance = hitchance->add_combobox(myhero->get_model() + ".hitchance.w", "Hitchance W", { {"Low",nullptr},{"Medium",nullptr },{"High", nullptr},{"Very High",nullptr} }, 2);
                hitchance::e_hitchance = hitchance->add_combobox(myhero->get_model() + ".hitchance.e", "Hitchance E", { {"Low",nullptr},{"Medium",nullptr },{"High", nullptr},{"Very High",nullptr} }, 1);
            }

            auto draw_settings = main_tab->add_tab(myhero->get_model() + ".draw", "Drawings Settings");
            {
                draw_settings::draw_range_q = draw_settings->add_checkbox(myhero->get_model() + ".draw.q", "Draw Q range", true);
                draw_settings::draw_range_q->set_texture(myhero->get_spell(spellslot::q)->get_icon_texture());
                draw_settings::draw_range_w = draw_settings->add_checkbox(myhero->get_model() + ".draw.w", "Draw W range", true);
                draw_settings::draw_range_w->set_texture(myhero->get_spell(spellslot::w)->get_icon_texture());
                draw_settings::draw_range_e = draw_settings->add_checkbox(myhero->get_model() + ".draw.e", "Draw E range", true);
                draw_settings::draw_range_e->set_texture(myhero->get_spell(spellslot::e)->get_icon_texture());
                draw_settings::draw_range_r = draw_settings->add_checkbox(myhero->get_model() + ".draw.r", "Draw R range", true);
                draw_settings::draw_range_r->set_texture(myhero->get_spell(spellslot::r)->get_icon_texture());
                if (flash)
                {
                    draw_settings::draw_flash_range = draw_settings->add_checkbox(myhero->get_model() + ".draw.flash", "Draw Flash range", true);
                    if (myhero->get_spell(spellslot::summoner1)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
                        draw_settings::draw_flash_range->set_texture(myhero->get_spell(spellslot::summoner1)->get_icon_texture());
                    else if (myhero->get_spell(spellslot::summoner2)->get_spell_data()->get_name_hash() == spell_hash("SummonerFlash"))
                        draw_settings::draw_flash_range->set_texture(myhero->get_spell(spellslot::summoner2)->get_icon_texture());
                }
            }
        }

        event_handler<events::on_update>::add_callback(on_update);
        event_handler<events::on_draw>::add_callback(on_draw);

    }

    void unload()
    {
        plugin_sdk->remove_spell(q);
        plugin_sdk->remove_spell(w);
        plugin_sdk->remove_spell(e);
        plugin_sdk->remove_spell(r);

        if (flash)
            plugin_sdk->remove_spell(flash);

        menu->delete_tab("veigar");

        event_handler<events::on_update>::remove_handler(on_update);
        event_handler<events::on_draw>::remove_handler(on_draw);
    }

    void on_update()
    {
        if (myhero->is_dead())
        {
            return;
        }

        if (orbwalker->can_move(0.05f))
        {
            if (r->is_ready() && combo::use_r->get_bool())
            {
                r_logic();
            }

            if (w->is_ready() && combo::use_w->get_bool())
            {
                auto target = target_selector->get_target(w->range(), damage_type::magical);

                if (target != nullptr)
                {
                    if (combo::w_auto_on_stun->get_bool())
                    {
                        if (w->cast(target, hit_chance::immobile))
                        {
                            return;
                        }
                    }

                    if (combo::w_auto_dashing->get_bool())
                    {
                        if (w->cast(target, hit_chance::dashing))
                        {
                            return;
                        }
                    }
                }
            }

            if (orbwalker->combo_mode())
            {
                if (q->is_ready() && combo::use_q->get_bool())
                {
                    q_logic();
                }

                if (e->is_ready() && combo::use_e->get_bool())
                {
                    e_logic();
                }

                if (w->is_ready() && combo::use_w->get_bool())
                {
                    w_logic();
                }
            }

            if (orbwalker->harass())
            {
                if (!myhero->is_under_enemy_turret())
                {
                    if (q->is_ready() && harass::use_q->get_bool())
                    {
                        q_logic();
                    }

                    if (w->is_ready() && harass::use_w->get_bool())
                    {
                        w_logic();
                    }
                }
            }

            if (orbwalker->flee_mode())
            {
                if (e->is_ready() && combo::use_e->get_bool())
                {
                    e_logic();
                }
            }

            if (orbwalker->mixed_mode() || orbwalker->lane_clear_mode())
            {
                auto lane_minions = entitylist->get_enemy_minions();
                auto monsters = entitylist->get_jugnle_mobs_minions();

                lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(q->range());
                    }), lane_minions.end());

                monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(q->range());
                    }), monsters.end());

                std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
                    {
                        return a->get_max_health() > b->get_max_health();
                    });

                if (!lane_minions.empty() && q->is_ready() && lasthit::use_q->get_bool())
                {
                    for (auto& minion : lane_minions)
                    {
                        if (q->get_damage(minion) > minion->get_health())
                        {
                            if (q->cast(minion, hit_chance::medium))
                            {
                                return;
                            }
                        }
                    }
                }
            }

            if (orbwalker->lane_clear_mode())
            {
                auto lane_minions = entitylist->get_enemy_minions();
                auto monsters = entitylist->get_jugnle_mobs_minions();

                lane_minions.erase(std::remove_if(lane_minions.begin(), lane_minions.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(q->range());
                    }), lane_minions.end());

                monsters.erase(std::remove_if(monsters.begin(), monsters.end(), [](game_object_script x)
                    {
                        return !x->is_valid_target(q->range());
                    }), monsters.end());

                std::sort(monsters.begin(), monsters.end(), [](game_object_script a, game_object_script b)
                    {
                        return a->get_max_health() > b->get_max_health();
                    });

                if (!lane_minions.empty())
                {
                    if (q->is_ready() && laneclear::use_q->get_bool())
                    {
                        if (q->cast(lane_minions.front(), hit_chance::medium))
                        {
                            return;
                        }
                    }

                    if (w->is_ready() && laneclear::use_w->get_bool())
                    {
                        if (w->cast(lane_minions.front(), hit_chance::medium))
                        {
                            return;
                        }
                    }
                }

                if (!monsters.empty())
                {
                    if (q->is_ready() && jungleclear::use_q->get_bool())
                    {
                        if (q->cast(monsters.front()))
                        {
                            return;
                        }
                    }

                    if (w->is_ready() && jungleclear::use_w->get_bool())
                    {
                        if (w->cast(monsters.front()))
                        {
                            return;
                        }
                    }
                }
            }
        }
    }

#pragma region q_logic
    void q_logic()
    {
        auto target = target_selector->get_target(q->range(), damage_type::magical);

        if (target != nullptr)
        {
            q->cast(target, get_hitchance(hitchance::q_hitchance));
        }
    }
#pragma endregion

#pragma region w_logic
    void w_logic()
    {
        auto target = target_selector->get_target(w->range(), damage_type::magical);

        if (target != nullptr)
        {
            w->cast(target, get_hitchance(hitchance::w_hitchance));
        }
    }
#pragma endregion

#pragma region e_logic
    void e_logic()
    {
        auto target = target_selector->get_target(e->range(), damage_type::magical);

        if (target != nullptr)
        {
            if (w->is_ready() || !combo::e_only_if_w_ready->get_bool())
            {
                e->cast(target, get_hitchance(hitchance::e_hitchance));
            }
        }
    }
#pragma endregion

#pragma region r_logic
    void r_logic()
    {
        if (flash && flash->is_ready() && combo::r_flash_above_r_range->get_bool())
        {
            auto target = target_selector->get_target(r->range() + flash->range(), damage_type::magical);
            if (target != nullptr && can_use_r_on(target) && target->get_distance(myhero) > r->range() + 50 && r->get_damage(target) > target->get_health())
            {
                flash->cast(target);
                r->cast(target);
                return;
            }
        }

        auto target = target_selector->get_target(r->range(), damage_type::magical);

        if (target != nullptr && can_use_r_on(target) && r->get_damage(target) > target->get_health())
        {
            r->cast(target);
        }
    }
#pragma endregion

    void on_draw()
    {

        if (myhero->is_dead())
        {
            return;
        }

        if (q->is_ready() && draw_settings::draw_range_q->get_bool())
            draw_manager->add_circle(myhero->get_position(), q->range(), Q_DRAW_COLOR);

        if (w->is_ready() && draw_settings::draw_range_w->get_bool())
            draw_manager->add_circle(myhero->get_position(), w->range(), W_DRAW_COLOR);

        if (e->is_ready() && draw_settings::draw_range_e->get_bool())
            draw_manager->add_circle(myhero->get_position(), e->range(), E_DRAW_COLOR);

        if (r->is_ready() && draw_settings::draw_range_r->get_bool())
            draw_manager->add_circle(myhero->get_position(), r->range(), R_DRAW_COLOR);

        if (flash && flash->is_ready() && draw_settings::draw_flash_range->get_bool())
            draw_manager->add_circle(myhero->get_position(), flash->range(), FLASH_DRAW_COLOR);

        for (auto& enemy : entitylist->get_enemy_heroes()) {
            if (!enemy->is_dead() && enemy->is_visible_on_screen() && r->is_ready())
            {
                draw_dmg_rl(enemy, r->get_damage(enemy), 4294929002);
                if (r->get_damage(enemy) > enemy->get_health())
                {
                    auto pos = enemy->get_position();
                    renderer->world_to_screen(pos, pos);
                    draw_manager->add_text_on_screen(pos + vector(20, 0), 4294929002 , 32, "KILLABLE");
                }
            }
        }
    }

    inline void draw_dmg_rl(game_object_script target, float damage, unsigned long color)
    {
        if (target != nullptr && target->is_valid() && target->is_hpbar_recently_rendered())
        {
            auto bar_pos = target->get_hpbar_pos();

            if (bar_pos.is_valid() && !target->is_dead() && target->is_visible())
            {
                const auto health = target->get_health();

                bar_pos = vector(bar_pos.x + (105 * (health / target->get_max_health())), bar_pos.y -= 10);

                auto damage_size = (105 * (damage / target->get_max_health()));

                if (damage >= health)
                {
                    damage_size = (105 * (health / target->get_max_health()));
                }

                if (damage_size > 105)
                {
                    damage_size = 105;
                }

                const auto size = vector(bar_pos.x + (damage_size * -1), bar_pos.y + 11);

                draw_manager->add_filled_rect(bar_pos, size, color);
            }
        }
    }

    bool can_use_r_on(game_object_script target)
    {
        auto it = combo::r_use_on.find(target->get_network_id());
        if (it == combo::r_use_on.end())
            return false;

        return it->second->get_bool();
    }

    hit_chance get_hitchance(TreeEntry* entry)
    {
        switch (entry->get_int())
        {
            case 0:
                return hit_chance::low;
                break;
            case 1:
            default:
                return hit_chance::medium;
                break;
            case 2:
                return hit_chance::high;
                break;
            case 3:
                return hit_chance::very_high;
                break;
         }
    }
};