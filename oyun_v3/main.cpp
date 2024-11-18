#include "icb_gui.h"
#include <stdlib.h>
#include <time.h>
#include <windows.h>

// **GLOBAL DEÐÝÞKENLER**
int FRM1;
int keypressed;
ICBYTES m;
HANDLE gameStateMutex;   // Oyun durumu için mutex
HANDLE renderMutex;      // Render iþlemleri için mutex

// Kaan uçak modeli için global ICBYTES tanýmý
ICBYTES kaanModel = { {10,1},{10,2},{9,3},{10,3},{11,3},{9,4},{10,4},{11,4},{8,5}, {9,5},{10,5},{11,5},
    {12,5},{8,6}, {9,6},{10,6},{11,6},{12,6},{8,7},{9,7},{10,7},{11,7},{12,7},{7,8},{8,8},{12,8},
    {13,8},{7,9},{8,9},{12,9},{13,9},{7,10},{8,10},{12,10},{13,10},{7,11},{8,11},{12,11},{13,11},
    {7,12},{8,12},{9,12},{11,12},{12,12},{13,12},{6,13},{7,13},{8,13},{9,13},{10,13},{11,13},{12,13},
    {13,13},{14,13},{5,14},{6,14},{7,14},{8,14},{9,14},{10,14},{11,14},{12,14},{13,14},{14,14},
    {15,14},{4,15},{5,15},{6,15},{7,15},{8,15},{9,15},{10,15},{11,15},{12,15},{13,15},{14,15},
    {15,15}, {16,15},{3,16},{4,16},{5,16},{6,16},{7,16},{8,16},{9,16},{10,16},{11,16},{12,16},
    {13,16},{14,16},{15,16},{16,16},{17,16},{2,17},{3,17},{4,17},{5,17},{6,17},{7,17},{8,17},
    {9,17},{10,17},{11,17},{12,17},{13,17},{14,17},{15,17},{16,17},{17,17},{18,17},{1,18},{2,18},
    {3,18},{4,18},{5,18},{6,18},{7,18},{8,18},{9,18},{11,18},{12,18},{13,18},{14,18},
    {15,18},{16,18},{17,18},{18,18},{19,18},{1,19},{2,19},{3,19},{4,19},{5,19},{6,19},{7,19},
    {8,19},{10,19},{12,19},{13,19},{14,19},{15,19},{16,19},{17,19},{18,19},{19,19},{1,20},{2,20},
    {3,20},{4,20},{5,20},{6,20},{7,20},{8,20},{10,20},{12,20},{13,20},{14,20},{15,20},{16,20},
    {17,20},{18,20},{19,20},{4,21},{5,21},{6,21},{7,21},{8,21},{9,21},{10,21},{11,21},{12,21},
    {13,21},{14,21},{15,21},{16,21},{7,22},{8,22},{9,22},{10,22},{11,22},{12,22},{13,22},
    {6,23},{7,23},{9,23},{10,23},{11,23},{13,23},{14,23},{5,24},{6,24},{7,24},{9,24},{10,24},
    {11,24},{13,24},{14,24},{15,24},{4,25},{5,25},{6,25},{8,25},{9,25},{10,25},{11,25},{12,25},
    {14,25},{15,25},{16,25},{4,26},{5,26},{6,26},{7,26},{8,26},{9,26},{10,26},{11,26},{12,26},
    {13,26},{14,26},{15,26},{16,26},{7,27},{8,27},{9,27},{11,27},{12,27},{13,27}
};
// **Struct Tanýmlarý**
struct Mermi {
    int x, y;
    HANDLE mutex;
    HANDLE thread;
    bool aktif;
};

struct DusenKutu {
    int x, y;
    int hizX, hizY;
    bool aktif;
    HANDLE mutex;
    HANDLE thread;
};

struct Slider {
    int x, y;
    int genislik, yukseklik;
    HANDLE mutex;
    HANDLE thread;
    ICBYTES model;
};

struct Oyun {
    bool devam;
    bool pause;
    Mermi mermi;
    DusenKutu kutu;
    Slider slider;
    time_t lastSeedTime;
    HANDLE renderThread;
};

Oyun oyun;

// Thread fonksiyonlarý için prototip tanýmlarý
DWORD WINAPI SliderThread(LPVOID lpParam);
DWORD WINAPI MermiThread(LPVOID lpParam);
DWORD WINAPI KutuThread(LPVOID lpParam);
DWORD WINAPI RenderThread(LPVOID lpParam);

void ICGUI_Create() {
    ICG_MWTitle("Threaded_Oyun_V2");
    ICG_MWSize(442, 550);
}

void WhenKeyPressed(int k) {
    keypressed = k;
}

// Yeni kutu oluþturma fonksiyonu

unsigned int SonRastgeleDeger = 0;
void yeniKutuOlustur(DusenKutu* kutu) {
    WaitForSingleObject(kutu->mutex, INFINITE);

    SYSTEMTIME st;
    GetSystemTime(&st);
    unsigned int newSeed = st.wMilliseconds + st.wSecond * 1000 + GetTickCount();

    newSeed ^= SonRastgeleDeger;
    srand(newSeed);

    int maxX = 380;
    int randomX = rand() % maxX;
    
    SonRastgeleDeger = randomX;

    kutu->x = randomX;
    kutu->y = -10;
    kutu->hizX = 0;
    kutu->hizY = 2;
    kutu->aktif = true;
    ReleaseMutex(kutu->mutex);
}

// Çarpýþma kontrolü
int carpismaDurum(int mermiX, int mermiY, DusenKutu* kutu) {
    WaitForSingleObject(kutu->mutex, INFINITE);
    int sonuc = -1;

    if (!(mermiY < kutu->y || mermiY > kutu->y + 30)) {
        int vurmaNoktasi = mermiX - kutu->x;
        if (vurmaNoktasi >= 0 && vurmaNoktasi < 9) sonuc = 0;
        else if (vurmaNoktasi >= 9 && vurmaNoktasi < 21) sonuc = 1;
        else if (vurmaNoktasi >= 21 && vurmaNoktasi < 30) sonuc = 2;
    }

    ReleaseMutex(kutu->mutex);
    return sonuc;
}

// Slider Thread
DWORD WINAPI SliderThread(LPVOID lpParam) {
    while (oyun.devam) {
        if (!oyun.pause) {
            WaitForSingleObject(oyun.slider.mutex, INFINITE);

            if (keypressed == 37) oyun.slider.x -= 2;
            if (keypressed == 39) oyun.slider.x += 2;

            if (oyun.slider.x < 5) oyun.slider.x = 5;
            if (oyun.slider.x > 380) oyun.slider.x = 380;

            ReleaseMutex(oyun.slider.mutex);
        }
        Sleep(16); // ~60 FPS
    }
    return 0;
}

// Mermi Thread
DWORD WINAPI MermiThread(LPVOID lpParam) {
    while (oyun.devam) {
        if (!oyun.pause) {
            WaitForSingleObject(oyun.mermi.mutex, INFINITE);

            // Mermi atýþý
            if (keypressed == 32 && oyun.mermi.y < 0) {
                WaitForSingleObject(oyun.slider.mutex, INFINITE);
                oyun.mermi.x = oyun.slider.x + 8;
                oyun.mermi.y = oyun.slider.y;
                ReleaseMutex(oyun.slider.mutex);
                keypressed = 0;
            }

            // Mermi hareketi
            if (oyun.mermi.y >= 0) {
                oyun.mermi.y -= 5;

                int vurmaBolgesi = carpismaDurum(oyun.mermi.x, oyun.mermi.y, &oyun.kutu);
                if (vurmaBolgesi != -1) {
                    WaitForSingleObject(oyun.kutu.mutex, INFINITE);
                    if (vurmaBolgesi == 0) {
                        oyun.kutu.hizX = 2;
                        oyun.kutu.hizY = -2;
                    }
                    else if (vurmaBolgesi == 2) {
                        oyun.kutu.hizX = -2;
                        oyun.kutu.hizY = -2;
                    }
                    else {
                        yeniKutuOlustur(&oyun.kutu);
                    }
                    ReleaseMutex(oyun.kutu.mutex);
                    oyun.mermi.y = -1;
                }
            }

            ReleaseMutex(oyun.mermi.mutex);
        }
        Sleep(16);
    }
    return 0;
}

// Kutu Thread
DWORD WINAPI KutuThread(LPVOID lpParam) {
    while (oyun.devam) {
        if (!oyun.pause) {
            WaitForSingleObject(oyun.kutu.mutex, INFINITE);

            if (oyun.kutu.aktif) {
                oyun.kutu.x += oyun.kutu.hizX;
                oyun.kutu.y += oyun.kutu.hizY;

                if (oyun.kutu.y > 400 || oyun.kutu.y < -30 ||
                    oyun.kutu.x < -30 || oyun.kutu.x > 400) {
                    yeniKutuOlustur(&oyun.kutu);
                }
            }

            ReleaseMutex(oyun.kutu.mutex);
        }
        Sleep(16);
    }
    return 0;
}

// Render Thread
DWORD WINAPI RenderThread(LPVOID lpParam) {
    while (oyun.devam) {
        if (!oyun.pause) {
            WaitForSingleObject(renderMutex, INFINITE);

            FillRect(m, 0, 0, 400, 400, 0x000000);

            // Mermi çizimi
            WaitForSingleObject(oyun.mermi.mutex, INFINITE);
            if (oyun.mermi.y >= 0) {
                FillRect(m, oyun.mermi.x, oyun.mermi.y, 1, 1, 0xFFFFFF);
            }
            ReleaseMutex(oyun.mermi.mutex);

            // Kutu çizimi
            WaitForSingleObject(oyun.kutu.mutex, INFINITE);
            if (oyun.kutu.aktif) {
                FillRect(m, oyun.kutu.x, oyun.kutu.y, 9, 30, 0xB8B8B8);
                FillRect(m, oyun.kutu.x + 9, oyun.kutu.y, 12, 30, 0x55FF55);
                FillRect(m, oyun.kutu.x + 21, oyun.kutu.y, 9, 30, 0xB8B8B8);
            }
            ReleaseMutex(oyun.kutu.mutex);

            // Kaan modeli çizimi (slider yerine)
            WaitForSingleObject(oyun.slider.mutex, INFINITE);
            SetPixels(kaanModel, oyun.slider.x, oyun.slider.y, 0xffff, m);
            ReleaseMutex(oyun.slider.mutex);

            DisplayImage(FRM1, m);
            ReleaseMutex(renderMutex);
        }
        Sleep(16);
    }
    return 0;
}

void butonfonk() {
    if (!oyun.devam) {
        oyun.devam = true;
        oyun.pause = false;

        // Mutex'leri oluþtur
        gameStateMutex = CreateMutex(NULL, FALSE, NULL);
        renderMutex = CreateMutex(NULL, FALSE, NULL);
        oyun.slider.mutex = CreateMutex(NULL, FALSE, NULL);
        oyun.mermi.mutex = CreateMutex(NULL, FALSE, NULL);
        oyun.kutu.mutex = CreateMutex(NULL, FALSE, NULL);

        // Ýlk kutuyu oluþtur
        yeniKutuOlustur(&oyun.kutu);

        // Thread'leri baþlat
        oyun.slider.thread = CreateThread(NULL, 0, SliderThread, NULL, 0, NULL);
        oyun.mermi.thread = CreateThread(NULL, 0, MermiThread, NULL, 0, NULL);
        oyun.kutu.thread = CreateThread(NULL, 0, KutuThread, NULL, 0, NULL);
        oyun.renderThread = CreateThread(NULL, 0, RenderThread, NULL, 0, NULL);
    }
    else {
        oyun.pause = !oyun.pause;
    }
    SetFocus(ICG_GetMainWindow());
}


void ICGUI_main() {
    FRM1 = ICG_FrameMedium(1, 1, 400, 400);
    ICG_SetOnKeyPressed(WhenKeyPressed);
    CreateImage(m, 400, 400, ICB_UINT);

    //kaan
    oyun.slider.x = 200;
    oyun.slider.y = 350;  
    oyun.slider.genislik = 19;  // Kaan modelinin geniþliði
    oyun.slider.yukseklik = 27; // Kaan modelinin yüksekliði
    oyun.mermi.y = -1;
    oyun.devam = false;
    oyun.pause = false;

    ICG_Button(130, 419, 120, 25, "START/PAUSE", butonfonk);
}