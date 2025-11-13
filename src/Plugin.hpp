// This file is included separately for each engine version

#include <WinBase.h>

namespace GOTHIC_NAMESPACE 
{

class OverlayHolder {
	public:
		void Clear() { overlays.clear(); }
		void Add(const zSTRING& o) { overlays.push_back(o); }
		void OnSave();
		void OnLoad();
		void OnUpdate();
		void Apply();

	private:
		std::vector<zSTRING> overlays;
};

static bool GetPrivateProfileBoolA(const LPCSTR lpAppName, const LPCSTR lpKeyName, const bool nDefault)
{
	TCHAR NPath[MAX_PATH];
    // Returns Gothic directory.
    int len = GetCurrentDirectory( MAX_PATH, NPath );
    // Get path to SystemPack.Ini
    auto ini = std::string( NPath, len ).append( "\\SystemPack.ini" );
	bool ret = GetPrivateProfileIntA( lpAppName, lpKeyName, nDefault, ini.c_str() ) ? true : false;
	Union::StringANSI::Format("zPreserveOverlays: Get setting {0}:{1} from {2}, with value: {3}.\n", lpAppName, lpKeyName, ini.c_str(), ret ? "true":"false").StdPrintLine();

	return ret;
}

static zCArchiver* CreateArchiverWrite(zSTRING path)
{
    static bool saveGameToAnisValid = false;
    static bool saveGameToAnsi;

    if (!saveGameToAnisValid) {
        saveGameToAnisValid = true;
		saveGameToAnsi = GetPrivateProfileBoolA("GAME", "SaveGameToANSI", false);
    }

    zTArchiveMode mode = saveGameToAnsi ? zARC_MODE_ASCII : zARC_MODE_BINARY_SAFE;

    zFILE* file = zfactory->CreateZFile(path);
    file->DirCreate();
    file->Create();
    delete file;

    return zarcFactory->CreateArchiverWrite(path, mode, 0, 0);
}

static zSTRING GetSavePath()
{
    zSTRING path = zoptions->GetDirString(DIR_ROOT);
    path += zoptions->GetDirString(DIR_SAVEGAMES);
    path += "current\\APPLIED_OVERLAYS.SAV";
    return path;
}

void OverlayHolder::OnSave()
{
	//Sometimes 1h overlay e.g. HUMANS_1HST3.MDS is loaded after shield overlay e.g. HUMANS_1HST2SH.MDS
	//Allow reordering, by priority

	//TODO: make reordering configurable, maybe by external from daedalus, or ini setting?
	// Move these overlays if found to the end in this order
	std::vector<zSTRING> specials = { "HUMANS_RAPIER_ST1.MDS", "HUMANS_RAPIER_ST2.MDS", "HUMANS_RAPIER_ST3.MDS", "HUMANS_1HST1SH.MDS", "HUMANS_1HST2SH.MDS", "HUMANS_1HST3SH.MDS", "HUM_2X2.MDS" };

	// First, stable_partition to move non-specials to the front
	auto it = std::stable_partition(overlays.begin(), overlays.end(), [&](const zSTRING& s) {
		return std::find(specials.begin(), specials.end(), s) == specials.end();
	});

	// Then, reorder the specials at the back in the desired order
	std::stable_sort(it, overlays.end(), [&](const zSTRING& a, const zSTRING& b) {
		return std::find(specials.begin(), specials.end(), a) <
			std::find(specials.begin(), specials.end(), b);
	});

	//create archiver
    zCArchiver* arc = CreateArchiverWrite(GetSavePath());

    //
    arc->WriteInt("SIZE", overlays.size());

    for (const zSTRING& overlay : overlays)
    {
        arc->WriteString("MDS", overlay);
    }
    //

    arc->Close();
    arc->Release();
}

void OverlayHolder::OnLoad()
{
    Clear();

    zCArchiver* arc = zarcFactory->CreateArchiverRead(GetSavePath(), 0);
    if (!arc)
        return;

    //
    int size = arc->ReadInt("SIZE");
    for (int i = 0; i < size; i++)
    {
        zSTRING overlay = arc->ReadString("MDS");
        Add(overlay);
    }
    //

    arc->Close();
    arc->Release();
    Apply();
}

void OverlayHolder::OnUpdate()
{
    if (!player) {
        return;
    }

    Clear();

    for (int i = 0; i < player->activeOverlays.GetNum(); i++) {
        Add(player->activeOverlays[i]);
    }
}

void OverlayHolder::Apply()
{
    for (const zSTRING& overlay : overlays) {
		Union::StringANSI::Format( "Applying player overlay: {}\n", overlay).StdPrintLine();
        player->ApplyOverlay(overlay);
    }
}

OverlayHolder g_overlays;

	void Game_EntryPoint()
	{

	}

	void Game_Init()
	{
		Union::StringANSI::Format("zPreserveOverlays.dll loaded.\n").StdPrintLine();
	}

	void Game_Exit()
	{

	}

	void Game_PreLoop()
	{

	}

	void Game_Loop()
	{
		//update overlays
		g_overlays.OnUpdate();
	}

	void Game_PostLoop()
	{

	}

	void Game_MenuLoop()
	{

	}

	void Game_SaveBegin()
	{
		//save overlays
    	g_overlays.OnSave();
	}

	void Game_SaveEnd()
	{

	}

	void LoadBegin()
	{

	}

	void LoadEnd()
	{
		//restore overlays
    	g_overlays.OnLoad();
	}

	void Game_LoadBegin_NewGame()
	{
		LoadBegin();
	}

	void Game_LoadEnd_NewGame()
	{
		LoadEnd();
	}

	void Game_LoadBegin_SaveGame()
	{
		LoadBegin();
	}

	void Game_LoadEnd_SaveGame()
	{
		LoadEnd();
	}

	void Game_LoadBegin_ChangeLevel()
	{
		LoadBegin();
	}

	void Game_LoadEnd_ChangeLevel()
	{
		LoadEnd();
	}

	void Game_LoadBegin_TriggerChangeLevel()
	{

	}

	void Game_LoadEnd_TriggerChangeLevel()
	{

	}

	void Game_Pause()
	{

	}

	void Game_Unpause()
	{

	}

	void Game_DefineExternals()
	{

	}

	void Game_ApplySettings()
	{

	}

	/*int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd);
	auto Hook_WinMain = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x004F3E10, 0x00506810, 0x005000F0, 0x00502D70)), &WinMain, Union::HookType::Hook_Detours);
	int __stdcall WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd)
	{
		Game_EntryPoint();
		return Hook_WinMain(hInstance, hPrevInstance, lpCmdLine, nShowCmd);
	}*/

	void __fastcall oCGame_Init(oCGame* self, void* vtable);
	auto Hook_oCGame_Init = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x00636F50, 0x0065D480, 0x006646D0, 0x006C1060)), &oCGame_Init, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Init(oCGame* self, void* vtable)
	{
		Hook_oCGame_Init(self, vtable);
		Game_Init();
	}

	/*void __fastcall CGameManager_Done(CGameManager* self, void* vtable);
	auto Hook_CGameManager_Done = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x00424850, 0x00427310, 0x004251A0, 0x004254E0)), &CGameManager_Done, Union::HookType::Hook_Detours);
	void __fastcall CGameManager_Done(CGameManager* self, void* vtable)
	{
		Game_Exit();
		Hook_CGameManager_Done(self, vtable);
	}*/

	/*void __fastcall oCGame_Render(oCGame* self, void* vtable);
	auto Hook_oCGame_Render = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063DBE0, 0x006648F0, 0x0066B930, 0x006C86A0)), &oCGame_Render, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Render(oCGame* self, void* vtable)
	{
		Game_PreLoop();
		Hook_oCGame_Render(self, vtable);
		Game_PostLoop();
	}*/


	void __fastcall oCGame_MainWorld_Render(Union::Registers& reg);
	auto Partial_zCWorld_Render = Union::CreatePartialHook(reinterpret_cast<void*>(zSwitch(0x0063DC76, 0x0066498B, 0x0066BA76, 0x006C87EB)), &oCGame_MainWorld_Render);
	void __fastcall oCGame_MainWorld_Render(Union::Registers& reg)
	{
		Game_Loop();
	}

	/*void __fastcall zCMenu_Render(zCMenu* self, void* vtable);
	auto Hook_zCMenu_Render = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x004D0DA0, 0x004E14E0, 0x004DB270, 0x004DDC20)), &zCMenu_Render, Union::HookType::Hook_Detours);
	void __fastcall zCMenu_Render(zCMenu* self, void* vtable)
	{
		Hook_zCMenu_Render(self, vtable);
		Game_MenuLoop();
	}*/

	void __fastcall oCGame_WriteSaveGame(oCGame* self, void* vtable, int slot, zBOOL saveGlobals);
	auto Hook_oCGame_WriteSaveGame = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063AD80, 0x00661680, 0x006685D0, 0x006C5250)), &oCGame_WriteSaveGame, Union::HookType::Hook_Detours);
	void __fastcall oCGame_WriteSaveGame(oCGame* self, void* vtable, int slot, zBOOL saveGlobals)
	{
		Game_SaveBegin();
		Hook_oCGame_WriteSaveGame(self, vtable, slot, saveGlobals);
		Game_SaveEnd();
	}

	/*void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, const zSTRING& levelPath);
	auto Hook_oCGame_LoadGame = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063C070, 0x00662B20, 0x00669970, 0x006C65A0)), &oCGame_LoadGame, Union::HookType::Hook_Detours);
	void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, const zSTRING& levelPath)
	{
		Game_LoadBegin_NewGame();
		Hook_oCGame_LoadGame(self, vtable, slot, levelPath);
		Game_LoadEnd_NewGame();
	}*/

	void __fastcall oCGame_LoadSaveGame(oCGame* self, void* vtable, int slot, zBOOL loadGlobals);
	auto Hook_oCGame_LoadSaveGame = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063C2A0, 0x00662D60, 0x00669BA0, 0x006C67D0)), &oCGame_LoadSaveGame, Union::HookType::Hook_Detours);
	void __fastcall oCGame_LoadSaveGame(oCGame* self, void* vtable, int slot, zBOOL loadGlobals)
	{
		Game_LoadBegin_SaveGame();
		Hook_oCGame_LoadSaveGame(self, vtable, slot, loadGlobals);
		Game_LoadEnd_SaveGame();
	}

	void __fastcall oCGame_ChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint);
	auto Hook_Game_Load_ChangeLevel = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063CD60, 0x00663950, 0x0066A660, 0x006C7290)), &oCGame_ChangeLevel, Union::HookType::Hook_Detours);
	void __fastcall oCGame_ChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint)
	{
		Game_LoadBegin_ChangeLevel();
		Hook_Game_Load_ChangeLevel(self, vtable, levelpath, startpoint);
		Game_LoadEnd_ChangeLevel();
	}

	/*void __fastcall oCGame_TriggerChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint);
	auto Hook_oCGame_TriggerChangeLevel = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063D480, 0x00664100, 0x0066AD80, 0x006C7AF0)), &oCGame_TriggerChangeLevel, Union::HookType::Hook_Detours);
	void __fastcall oCGame_TriggerChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint)
	{
		Game_LoadBegin_TriggerChangeLevel();
		Hook_oCGame_TriggerChangeLevel(self, vtable, levelpath, startpoint);
		Game_LoadEnd_TriggerChangeLevel();
	}*/

/*#if ENGINE <= Engine_G1A
	void __fastcall oCGame_Pause_G1(oCGame* self, void* vtable);
	auto Hook_oCGame_Pause = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063DF50, 0x00664CD0, 0, 0)), &oCGame_Pause_G1, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Pause_G1(oCGame* self, void* vtable)
	{
		Hook_oCGame_Pause(self, vtable);
		Game_Pause();
	}
#else
	void __fastcall oCGame_Pause_G2(oCGame* self, void* vtable, zBOOL sessionPaused);
	auto Hook_oCGame_Pause = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0, 0, 0x0066BD50, 0x006C8AD0)), &oCGame_Pause_G2, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Pause_G2(oCGame* self, void* vtable, zBOOL sessionPaused)
	{
		Hook_oCGame_Pause(self, vtable, sessionPaused);
		Game_Pause();
	}
#endif*/

	/*void __fastcall oCGame_Unpause(oCGame* self, void* vtable);
	auto Hook_oCGame_Unpause = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063E1D0, 0x00664F80, 0x0066BFD0, 0x006C8D50)), &oCGame_Unpause, Union::HookType::Hook_Detours);
	void __fastcall oCGame_Unpause(oCGame* self, void* vtable)
	{
		Hook_oCGame_Unpause(self, vtable);
		Game_Unpause();
	}*/

	/*void __fastcall oCGame_DefineExternals_Ulfi(oCGame* self, void* vtable, zCParser* parser);
	auto Hook_oCGame_DefineExternals_Ulfi = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x006495B0, 0x006715F0, 0x00677A00, 0x006D4780)), &oCGame_DefineExternals_Ulfi, Union::HookType::Hook_Detours);
	void __fastcall oCGame_DefineExternals_Ulfi(oCGame* self, void* vtable, zCParser* parser)
	{
		Hook_oCGame_DefineExternals_Ulfi(self, vtable, parser);
		Game_DefineExternals();
	}*/

	/*void __fastcall CGameManager_ApplySomeSettings(CGameManager* self, void* vtable);
	auto Hook_CGameManager_ApplySomeSettings = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x004267C0, 0x004291E0, 0x00427370, 0x004276B0)), &CGameManager_ApplySomeSettings, Union::HookType::Hook_Detours);
	void __fastcall CGameManager_ApplySomeSettings(CGameManager* self, void* vtable)
	{
		Hook_CGameManager_ApplySomeSettings(self, vtable);
		Game_ApplySettings();
	}*/
}