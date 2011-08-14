#ifndef GSDH
#define GSDH

#ifdef __cplusplus
extern "C" {
#endif

#define GSD_APIVER 0x00020001

struct GSD_TextureInfo
{
	BOOL active;    // テクスチャを表示するか否か (TRUE、FALSE)
	DWORD texSize;  // テクスチャのサイズ、GSD_InitTexture() で指定した値と同じでなければならない
	int x, y;       // テクスチャの位置 (alignの値によって基準位置が変わる)
	DWORD color;    // 基本色 (通常は不透明の白 0xffffffff)
	DWORD align;    // テクスチャの配置 (DT_LEFT、DT_CENTER、DT_RIGHT、DT_TOP、DT_VCENTER、DT_BOTTOM)
	BYTE *data;     // テクスチャを格納したバッファ 設定のみ変更する場合はNULL
};

BOOL WINAPI GSD_Initialize();
void WINAPI GSD_Finalize();
int WINAPI GSD_GetMaxTextureNum();
int WINAPI GSD_GetMaxTextureSize();
DWORD WINAPI GSD_GetLastUpdate();
DWORD WINAPI GSD_GetVersion();
DWORD WINAPI GSD_GetApiVersion();
BOOL WINAPI GSD_CheckApiVersion(DWORD ver);
BOOL WINAPI GSD_InitTexture(const DWORD *texSize, int num);
BOOL WINAPI GSD_SetTexture(int index, const struct GSD_TextureInfo *info);
BOOL WINAPI GSD_GetTexture(int index, struct GSD_TextureInfo *info);
void WINAPI GSD_DelTexture(int index);
void WINAPI GSD_SetTimerOrg(__int64 org);
void WINAPI GSD_SetFpsLimit(double fps);
void WINAPI GSD_SetFpsAlign(DWORD align);
void WINAPI GSD_ShowFps(BOOL show);
void WINAPI GSD_UpdateOnActive(BOOL uoa);
void WINAPI GSD_DataLock();
void WINAPI GSD_DataUnlock();

#ifdef __cplusplus
}

class GSD_DataLocker
{
public:
	GSD_DataLocker()
	{
		GSD_DataLock();
	}

	~GSD_DataLocker()
	{
		GSD_DataUnlock();
	}
};

#endif

#endif
