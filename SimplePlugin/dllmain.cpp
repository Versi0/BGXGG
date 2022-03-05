#include "../plugin_sdk/plugin_sdk.hpp"

PLUGIN_NAME("Yordle AIO");
SUPPORTED_CHAMPIONS(champion_id::Veigar);

#include "veigar.h"

PLUGIN_API bool on_sdk_load(plugin_sdk_core* plugin_sdk_good)
{
    DECLARE_GLOBALS(plugin_sdk_good);

    switch (myhero->get_champion())
    {
        case champion_id::Veigar:
            veigar::load();
            break;
        default:
            return false;
    }

    return true;
}

PLUGIN_API void on_sdk_unload()
{
    switch (myhero->get_champion())
    {
        case champion_id::Veigar:
            veigar::unload();
            break;
        default:
            break;
    }
}