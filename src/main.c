#include <stdio.h>
#include <swilib.h>
#include <stdlib.h>
#include <string.h>
#include "ui/ui.h"
#include "tz.h"
#include "world_clock.h"

typedef struct {
    CSM_RAM csm;
    TZ_EXTENDED **pools;
    int pools_size;
    int gui_id;
} MAIN_CSM;

const int minus11 = -11;
unsigned short maincsm_name_body[140];

int ComparePools(const void *tz1, const void *tz2) {
    WSHDR ws1, ws2;
    uint16_t wsbody1[64], wsbody2[64];
    CreateLocalWS(&ws1, wsbody1, 64);
    CreateLocalWS(&ws2, wsbody2, 64);
    wsprintf(&ws1, "%t", ((TZ_EXTENDED*)tz1)->tz.lgp_id);
    wsprintf(&ws2, "%t", ((TZ_EXTENDED*)tz2)->tz.lgp_id);
    return wstrcmp(&ws1, &ws2);
}

int AddElementToPool(TZ_EXTENDED **pool, const TZ *tz, int *pool_size) {
    int size = *pool_size;
    TZ_EXTENDED *new_pool = realloc(*pool, sizeof(TZ_EXTENDED) * (size + 2));
    if (!new_pool) {
        return 0;
    }
    TZ_EXTENDED *p = NULL;
    p = &(new_pool[size]);
    memcpy(p, tz, sizeof(TZ));
    p->id = GetTimeZoneByGMT(p->tz.gmt);
    size++;
    p = &(new_pool)[size];
    p->id = -1;
    *pool = new_pool;
    *pool_size = size;
    return 1;
}

void FinalizePool(TZ_EXTENDED ***pools, int *pools_size, TZ_EXTENDED **pool, int *pool_size) {
    int size = *pools_size;
    TZ_EXTENDED **new_pools = realloc(*pools, sizeof(TZ_EXTENDED*) * (size + 1));
    if (new_pools) {
        qsort(*pool, *pool_size, sizeof(TZ_EXTENDED), ComparePools);
        new_pools[size] = *pool;
        size++;
        *pools = new_pools;
        *pools_size = size;
    } else {
        mfree(*pool);
    }
    *pool = NULL;
    *pool_size = 0;
}

int GetPools(TZ_EXTENDED ***pools) {
    TZ *tz = RamTimeZones();

    int pools_size = 0;
    int pool_size = 0;
    TZ_EXTENDED *pool = NULL;
    while (tz->gmt) {
        if (AddElementToPool(&pool, tz, &pool_size)) {
            TZ *next = tz + 1;
            if (next->gmt) {
                if (strncmp(tz->gmt, next->gmt, 8) != 0) {
                    FinalizePool(pools, &pools_size, &pool, &pool_size);
                }
            } else {
                FinalizePool(pools, &pools_size, &pool, &pool_size);
            }
        }
        tz++;
    }
    if (pool_size > 0) {
        FinalizePool(pools, &pools_size, &pool, &pool_size);
    }
    return pools_size;
}

static void OnCreate(CSM_RAM *data) {
    MAIN_CSM *csm = (MAIN_CSM*)data;
    csm->pools = NULL;
    csm->pools_size = GetPools(&(csm->pools));
    int current_city = ReadCurrentCity();
    if (current_city == -1) {
        current_city = 0x60; //Moscow
    }
    csm->gui_id = CreateUI(csm->pools, csm->pools_size, current_city);
}

static void OnClose(CSM_RAM *data) {
    MAIN_CSM *csm = (MAIN_CSM*)data;
    for (int i = 0; i < csm->pools_size; i++) {
        mfree(csm->pools[i]);
    }
    mfree(csm->pools);
    SUBPROC(kill_elf);
}

static int OnMessage(CSM_RAM *data, GBS_MSG *msg) {
    MAIN_CSM *csm = (MAIN_CSM*)data;
    if ((msg->msg == MSG_GUI_DESTROYED) && ((int)msg->data0 == csm->gui_id)) {
        csm->csm.state = -3;
    }
    return 1;
}

static const struct {
    CSM_DESC maincsm;
    WSHDR maincsm_name;
} MAINCSM = {
    {
        OnMessage,
        OnCreate,
#ifdef NEWSGOLD
        0,
        0,
        0,
        0,
#endif
        OnClose,
        sizeof(MAIN_CSM),
        1,
        &minus11
    },
    {
    maincsm_name_body,
    NAMECSM_MAGIC1,
    NAMECSM_MAGIC2,
    0x0,
    139,
    0
    }
};

void UpdateCSMname(void) {
    wsprintf((WSHDR *)&MAINCSM.maincsm_name, "%s", "Time zones");
}

int main() {
    MAIN_CSM main_csm = { 0 };
    LockSched();
    UpdateCSMname();
    CreateCSM(&MAINCSM.maincsm, &main_csm, 0);
    UnlockSched();
    return 0;
}
