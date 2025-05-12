#include <stdio.h>
#include <swilib.h>
#include <string.h>
#include <stdlib.h>
#include "../tz.h"
#include "../world_clock.h"

typedef struct {
    TZ_EXTENDED **pools;
    int pools_size;
    int current_city;

    int pool_size;
    int pool_id;
    int city_id;
    int menu_offset;
    int allow_scroll_up;
    int allow_scroll_down;

    int timer_time;
} UI_DATA;

static int ICON[] = {1644};

static HEADER_DESC HEADER_D = {{0, 0, 0, 0}, ICON, (int)"Time zones", (int)LGP_NULL};

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

static void OnRedraw(void *) {
    TDate date;
    TTime time;
    void *gui = GetTopGUI();
    UI_DATA *data = TViewGetUserPointer(gui);
    GetDateTime(&date, &time);


    WSHDR ws, ws2;
    char str[128];
    uint16_t wsbody[128], wsbody2[128];
    CreateLocalWS(&ws, wsbody, 128);
    CreateLocalWS(&ws2, wsbody2, 128);

    const RECT *main_area_rect = GetMainAreaRECT();
    const TZ_EXTENDED *tz = &(data->pools[data->pool_id][data->city_id]);

    int x = 0, x2 = 0;
    int y = main_area_rect->y, y2 = 0;
    int x_bg = -240 + 60 + (data->pool_id * -1) * 5;
    int x_bg2 = 60 + (data->pool_id * -1) * 5;
    int x_bg3 = 60 + 240 + (data->pool_id * -1) * 5;
    DrawImg(x_bg, y, 1606);
    DrawImg(x_bg2, y, 1606);
    DrawImg(x_bg3, y, 1606);

    int pic = 1607 + data->pool_id;
    if (data->pool_id == 0) {
        x = x_bg;
    } else {
        x = x_bg2;
    }
    DrawImg(x, y, pic);

    x = 0; x2 = ScreenW() - 1;
    y += GetImgHeight(1606); y2 = y + GetFontYSIZE(FONT_SMALL) + 1;
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

    y = y2 + 3;
    y2 = y + GetFontYSIZE(FONT_SMALL);
    for (int i = data->menu_offset; i < data->pool_size; i++) {
        tz = &(data->pools[data->pool_id][i]);

        if (i == data->city_id) {
            DrawRectangle(0, y, ScreenW() - 1, y2 + 1, RECT_DOT_OUTLINE,
                         GetPaletteAdrByColorIndex(PC_FOREGROUND), GetPaletteAdrByColorIndex(0x23));
        }
        wsprintf(&ws, "%t%c%c", tz->tz.lgp_id, UTF16_ALIGN_RIGHT, UTF16_ALIGN_NONE);
        if (tz->tz.city_id == data->current_city) {
            wsInsertChar(&ws, UTF16_FONT_SMALL_BOLD, 1);
            wsInsertChar(&ws, UTF16_FONT_RESET, wstrlen(&ws));
        }
        if (IsSummerTime(tz->id)) {
            wsInsertChar(&ws, 0xE465, wstrlen(&ws));
        }
        DrawString(&ws, x, y + 1, x2, y2, FONT_SMALL, 0,
                    GetPaletteAdrByColorIndex(PC_FOREGROUND), GetPaletteAdrByColorIndex(0x17));
        y += GetFontYSIZE(FONT_SMALL) + 3;
        y2 += GetFontYSIZE(FONT_SMALL) + 3;
    }
}

void SetScrollFlags(UI_DATA *data) {
    if (data->pool_size >= 3) {
        data->allow_scroll_up = ((data->menu_offset && data->city_id - 1 < data->menu_offset) || data->city_id <= 0) ? 1 : 0;
        data->allow_scroll_down = (data->city_id >= 2 && data->city_id - 1 > data->menu_offset) ? 1 : 0;
    } else {
        data->allow_scroll_up = 0;
        data->allow_scroll_down = 0;
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
        char message[32];
        uint16_t wsbody[32];
        CreateLocalWS(&ws, wsbody, 32);
        wsprintf(&ws, "Time zone set to: %t", tz->tz.lgp_id);
        ws_2str(&ws, message, 31);
        ShowMSG(1, (int)message);
    }
    else if (msg->gbsmsg->msg == KEY_DOWN || msg->gbsmsg->msg == LONG_PRESS) {
        if (msg->gbsmsg->submess == LEFT_BUTTON) {
            data->pool_id--;
            if (data->pool_id < 0) {
                data->pool_id = data->pools_size - 1;
            }
            data->city_id = 0;
            data->menu_offset = 0;
            DirectRedrawGUI();
        } else if (msg->gbsmsg->submess == RIGHT_BUTTON) {
            data->pool_id++;
            if (data->pool_id >= data->pools_size) {
                data->pool_id = 0;
            }
            data->city_id = 0;
            data->menu_offset = 0;
            DirectRedrawGUI();
        } else if (msg->gbsmsg->submess == UP_BUTTON) {
            data->city_id--;
            if (data->allow_scroll_up) {
                data->menu_offset--;
            }
            if (data->city_id < 0) {
                data->city_id = data->pool_size - 1;
                if (data->allow_scroll_up) {
                    data->menu_offset = data->pool_size - 3;
                }
            }
            DirectRedrawGUI();
        } else if (msg->gbsmsg->submess == DOWN_BUTTON) {
            data->city_id++;
            if (data->allow_scroll_down) {
                data->menu_offset++;
            }
            if (data->pools[data->pool_id][data->city_id].id == -1) {
                data->city_id = 0;
                data->menu_offset = 0;
            }
            DirectRedrawGUI();
        }
    }
    return -1;
}

void *UpdateHeader(GUI *gui) {
    TTime time;
    void *header = GetHeaderPointer(gui);
    WSHDR *ws = AllocWS(32);
    GetDateTime(NULL, &time);
    GetTime_ws(ws, &time, 0x227);
    SetHeaderText(header, ws, malloc_adr(), mfree_adr());
    return header;
}

void UpdateHeaderProc(void *gui) {
    UI_DATA *data = TViewGetUserPointer(gui);
    void *header = UpdateHeader(gui);
    GUI_METHODS **m = GetDataOfItemByID(gui, 2);
    m[1]->onRedraw(header);
    GUI_StartTimerProc(gui, data->timer_time, 1000, UpdateHeaderProc);
}

static void GHook(GUI *gui, int cmd) {
    UI_DATA *data = TViewGetUserPointer(gui);

    if (cmd == TI_CMD_REDRAW) {
        data->pool_size = GetPoolSize(data->pools[data->pool_id]);
        SetScrollFlags(data);
        UpdateHeader(gui);
    } else if (cmd == TI_CMD_CREATE) {
        static GUI_METHODS gui_methods;
        void **m = GetDataOfItemByID(gui, 4);
        memcpy(&gui_methods, m[1], sizeof(GUI_METHODS));
        gui_methods.onRedraw = (void*)(GUI*)OnRedraw;
        m[1] = &gui_methods;

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
        if (data->city_id >= 2) {
            data->menu_offset = (data->city_id < data->pool_size - 1) ? data->city_id - 1 : data->pool_size - 3;
        }
    } else if (cmd == TI_CMD_FOCUS) {
        data->timer_time = GUI_NewTimer(gui);
        GUI_StartTimerProc(gui, data->timer_time, 1000, UpdateHeaderProc);
    } else if (cmd == TI_CMD_UNFOCUS) {
        GUI_DeleteTimer(gui, data->timer_time);
    } else if (cmd == TI_CMD_DESTROY) {
        GUI_DeleteTimer(gui, data->timer_time);
    }
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


int CreateUI(TZ_EXTENDED **pools, int pools_size, int current_city) {
    memcpy(&(HEADER_D.rc), GetHeaderRECT(), sizeof(RECT));
    memcpy(&(TVIEW_D.rc), GetMainAreaRECT(), sizeof(RECT));

    UI_DATA *data = malloc(sizeof(UI_DATA));
    zeromem(data, sizeof(UI_DATA));
    data->pools = pools;
    data->pools_size = pools_size;
    data->current_city = current_city;

    void *gui = TViewGetGUI(malloc_adr(), mfree_adr());
    TViewSetDefinition(gui, &TVIEW_D);
    SetHeaderToMenu(gui, &HEADER_D, malloc_adr());

    WSHDR *ws = AllocWS(1);
    TViewSetText(gui, ws, malloc_adr(), mfree_adr());
    TViewSetUserPointer(gui, data);

    return CreateGUI(gui);
}
