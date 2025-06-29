#include <stdio.h>
#include <swilib.h>
#include <string.h>
#include <stdlib.h>
#include "../tz.h"
#include "../world_clock.h"
#include "icons.h"

typedef struct {
    TZ_EXTENDED **pools;
    int pools_size;
    int current_city;

    WIDGET *menu;
    int pool_size;
    int pool_id;
    int city_id;
} UI_DATA;

#define MENU_WIDGET_ID 20

GUI_METHODS METHODS;

const int SOFTKEYS[] = {SET_LEFT_SOFTKEY, SET_MIDDLE_SOFTKEY, SET_RIGHT_SOFTKEY};

static SOFTKEY_DESC SOFTKEY_D[] = {
    {0x0018, 0x0000, (int)"Select"},
    {0x0018, 0x0000, (int)LGP_DOIT_PIC},
    {0x0001, 0x0000, (int)"Exit"},
};

static SOFTKEYSTAB SOFTKEYS_TAB = {
    SOFTKEY_D, 3
};

void GetCityTime(const TDate *date, const TTime *time, int time_zone, TTime *out_time) {
    int seconds = GetSecondsFromTime(time) - GetTimeZoneShift(date, time, GetCurrentTimeZone()) * 60;
    seconds += GetTimeZoneShift(date, time, time_zone) * 60;
    GetTimeFromSeconds(out_time, seconds);
}

int GetPoolSize(const TZ_EXTENDED *pool) {
    int size = 0;
    while (pool->id != -1) {
        pool++;
        size++;
    }
    return size;
}

static void OnRedraw(GUI *gui) {
    WIDGET *menu = GetDataOfItemByID(gui, MENU_WIDGET_ID);
    menu->methods->onRedraw(menu);

    TDate date;
    TTime time;
    UI_DATA *data = TViewGetUserPointer(gui);
    GetDateTime(&date, &time);

    WSHDR ws, ws2;
    char str[128];
    uint16_t wsbody[128], wsbody2[128];
    CreateLocalWS(&ws, wsbody, 128);
    CreateLocalWS(&ws2, wsbody2, 128);

    const TZ_EXTENDED *tz = &(data->pools[data->pool_id][data->city_id]);

    int x = 0, x2 = 0;
    int y = YDISP, y2 = 0;
    int x_bg = -240 + 60 + (data->pool_id * -1) * 5;
    int x_bg2 = 60 + (data->pool_id * -1) * 5;
    int x_bg3 = 60 + 240 + (data->pool_id * -1) * 5;
    DrawImg(x_bg, y, ICON_MAP);
    DrawImg(x_bg2, y, ICON_MAP);
    DrawImg(x_bg3, y, ICON_MAP);

    int pic = ICON_MAP + 1 + data->pool_id;
    if (data->pool_id == 0) {
        x = x_bg;
    } else {
        x = x_bg2;
    }
    DrawImg(x, y, pic);

    x = gui->canvas->x; x2 = gui->canvas->x2;
    y += GetImgHeight(ICON_MAP); y2 = y + GetFontYSIZE(FONT_SMALL) + 1;
    DrawRectangle(x, y, x2, y2, 0,
                 GetPaletteAdrByColorIndex(PC_FOREGROUND), GetPaletteAdrByColorIndex(PC_FOREGROUND));

    x = 3; x2 = x2 - 3;
    strncpy(str, tz->tz.gmt + 3, 6);
    str[6] = '\0';

    TTime city_time;
    GetCityTime(&date, &time, tz->id, &city_time);
    GetTime_ws(&ws2, &city_time, 0x223);
    wsprintf(&ws, "UTC%s%c%w%c", str, UTF16_ALIGN_RIGHT, &ws2, UTF16_ALIGN_NONE);
    DrawString(&ws, x, y, x2, y2, FONT_SMALL, 0,
           GetPaletteAdrByColorIndex(PC_BACKGROUND), GetPaletteAdrByColorIndex(0x17));

    TVIEW_DESC *desc = gui->definition;
    if (desc->global_hook_proc) {
        desc->global_hook_proc(gui, TI_CMD_REDRAW);
    }
    WIDGET *softkeys = GetDataOfItemByID(gui, 0);
    if (softkeys) {
        softkeys->methods->onRedraw(softkeys);
    }
}

static int OnKey(GUI *gui, GUI_MSG *msg) {
    UI_DATA *data = TViewGetUserPointer(gui);

    if (msg->keys == 0x01) {
        return 1;
    }
    else if (msg->keys == 0x18) {
        TZ_EXTENDED *tz = &(data->pools[data->pool_id][data->city_id]);
        SetTimeZone(tz->id);
        data->current_city = tz->tz.city_id;
        WriteCurrentCity(data->current_city);

        WSHDR ws;
        char message[64];
        uint16_t wsbody[64];
        CreateLocalWS(&ws, wsbody, 61);
        wsprintf(&ws, "Time zone set to: %t", tz->tz.lgp_id);
        ws_2str(&ws, message, 63);
        ShowMSG(1, (int)message);
    }
    else if (msg->gbsmsg->msg == KEY_DOWN || msg->gbsmsg->msg == LONG_PRESS) {
        if (msg->gbsmsg->submess == LEFT_BUTTON) {
            data->pool_id--;
            if (data->pool_id < 0) {
                data->pool_id = data->pools_size - 1;
            }
            data->city_id = 0;
            data->pool_size = GetPoolSize(data->pools[data->pool_id]);
            SetCursorToMenuItem(data->menu, data->city_id);
            Menu_SetItemCountDyn(data->menu, data->pool_size);
            DirectRedrawGUI();
        } else if (msg->gbsmsg->submess == RIGHT_BUTTON) {
            data->pool_id++;
            if (data->pool_id >= data->pools_size) {
                data->pool_id = 0;
            }
            data->city_id = 0;
            data->pool_size = GetPoolSize(data->pools[data->pool_id]);
            SetCursorToMenuItem(data->menu, data->city_id);
            Menu_SetItemCountDyn(data->menu, data->pool_size);
            DirectRedrawGUI();
        } else if (msg->gbsmsg->submess == UP_BUTTON) {
            data->city_id = GetCurMenuItem(data->menu) - 1;
            if (data->city_id < 0) {
                data->city_id = data->pool_size - 1;
            }
            SetCursorToMenuItem(data->menu, data->city_id);
            RefreshGUI();
        } else if (msg->gbsmsg->submess == DOWN_BUTTON) {
            data->city_id = GetCurMenuItem(data->menu) + 1;
            if (data->pools[data->pool_id][data->city_id].id == -1) {
                data->city_id = 0;
            }
            SetCursorToMenuItem(data->menu, data->city_id);
            RefreshGUI();
        }
    }
    return -1;
}

static void GHook(GUI *gui, int cmd) {
    UI_DATA *data = TViewGetUserPointer(gui);

    if (cmd == TI_CMD_CREATE) {
        for (int i = 0; i < data->pools_size; i++) {
            for (int j = 0 ; j < GetPoolSize(data->pools[i]); j++) {
                if (data->pools[i][j].tz.city_id == data->current_city) {
                    data->pool_id = i;
                    data->city_id = j;
                    break;
                }
            }
        }
        data->pool_size = GetPoolSize(data->pools[data->pool_id]);
        Menu_SetItemCountDyn(data->menu, data->pool_size);
        SetCursorToMenuItem(data->menu, data->city_id);
    } else if (cmd == TI_CMD_DESTROY) {
        mfree(data);
    }
}

void ItemProc(void *gui, int item_n, void *user_pointer) {
    UI_DATA *data = user_pointer;

    static int icons_checked[] = {ICON_RADIO_CHECKED, 0};
    static int icons_unchecked[] = {ICON_RADIO_UNCHECKED, 0};
    TZ_EXTENDED *tz = &(data->pools[data->pool_id][item_n]);

    void *item = AllocMenuItem(gui);
    WSHDR *ws = AllocMenuWS(gui, 128);
    SetMenuItemIconArray(gui, item, (tz->tz.city_id == data->current_city) ? icons_checked : icons_unchecked);
    SetMenuItemIcon(gui, item_n, 0);
    wsprintf(ws, "%t", tz->tz.lgp_id);
    if (IsSummerTime(tz->id)) {
        wstrcatprintf(ws, " %c%c%c%c%c", UTF16_ALIGN_RIGHT, UTF16_FONT_SMALL, 0xE465, UTF16_FONT_RESET, UTF16_ALIGN_NONE);
    }
    SetMenuItemText(gui, item, ws, item_n);
}

static TVIEW_DESC TVIEW_D = {
    8,
    OnKey,
    GHook,
    NULL,
    SOFTKEYS,
    &SOFTKEYS_TAB,
    {0, 0, 0, 0},
    FONT_SMALL,
    0x64,
    0x65,
    0,
    0,
};


static MENU_DESC MENU_D = {
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    MENU_FLAGS_ENABLE_ICONS,
    ItemProc,
    NULL,
    NULL,
    0,
};

int CreateUI(TZ_EXTENDED **pools, int pools_size, int current_city) {
    RECT *main_area_rect = GetMainAreaRECT();

    memcpy(&(TVIEW_D.rc), main_area_rect, sizeof(RECT));

    void *ma = malloc_adr();
    void *mf = mfree_adr();

    UI_DATA *data = malloc(sizeof(UI_DATA));
    zeromem(data, sizeof(UI_DATA));
    data->pools = pools;
    data->pools_size = pools_size;
    data->current_city = current_city;

    GUI *gui = TViewGetGUI(ma, mf);
    TViewSetDefinition(gui, &TVIEW_D);

    WSHDR *ws = AllocWS(1);
    TViewSetText(gui, ws, ma, mf);
    TViewSetUserPointer(gui, data);

    data->menu = GetMenuGUI(ma, mf);
    SetMenuToGUI(data->menu, &MENU_D);
    int y = YDISP + GetImgHeight(ICON_MAP) + GetFontYSIZE(FONT_SMALL);
    SetMenuRect(data->menu, main_area_rect->x, y + GetFontYSIZE(FONT_MEDIUM) / 3,
                main_area_rect->x2, y - GetFontYSIZE(FONT_MEDIUM) * 3);
    MenuSetUserPointer(data->menu, data);
    AttachWidget(gui, data->menu, MENU_WIDGET_ID, ma);

    memcpy(&METHODS, gui->methods, sizeof(GUI_METHODS));
    METHODS.onRedraw = OnRedraw;
    gui->methods = &METHODS;

    return CreateGUI(gui);
}
