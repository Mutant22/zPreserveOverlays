// This file is included separately for each engine version

#include <WinBase.h>
#include <Union/Hook.h>
#include <optional>

namespace GOTHIC_NAMESPACE {

std::vector<std::string> GetIniOverlaysList(const char* section, const char* key)
{
	std::vector<std::string> result;
	TCHAR NPath[MAX_PATH];
	// Returns Gothic directory.
	int len = GetCurrentDirectory(MAX_PATH, NPath);

	// Get -GAME:modname.ini from command line, and use it for retrieving Overlay List
	zSTRING& cmdline = zoptions->commandline;
	std::string tmp(cmdline.ToChar());
	std::transform(tmp.begin(), tmp.end(), tmp.begin(), ::toupper);
	size_t cmdline_pos_start = tmp.find("-GAME:");
	size_t cmdline_pos_stop = tmp.find(".INI");
	if (cmdline_pos_start != std::string::npos && cmdline_pos_stop != std::string::npos) {
		tmp = std::string(tmp.data() + cmdline_pos_start + sizeof("-GAME:") - 1, tmp.data() + cmdline_pos_stop + sizeof(".INI") - 1);
	}
	else {
		// Fallback
		tmp = "GOTHIC.INI";
	}

	// Get path to mod ini configuration file
	auto ini = std::string(NPath, len).append("\\system\\").append(tmp);
	char ret_string[200];
	DWORD ret = GetPrivateProfileStringA(section, key, "", ret_string, sizeof(ret_string), ini.c_str());
	if (ret == 0) {
		// Fallback 2
		ini = std::string(NPath, len).append("\\system\\SystemPack.ini");
		ret = GetPrivateProfileStringA(section, key, "", ret_string, sizeof(ret_string), ini.c_str());
	}
	if (ret) {
		std::string str(ret_string);
		std::transform(str.begin(), str.end(), str.begin(), ::toupper);

		//Remove comments starting by # or ;
		size_t comment_pos = str.find_first_of("#;");
		if (comment_pos != std::string::npos) {
			str.resize(comment_pos);
		}

		//Remove whitespace from start and back
		// trim from left
		const char* to_trim = " \t\n\r\f\v";
		str.erase(0, str.find_first_not_of(to_trim));
		// trim from right
		str.erase(str.find_last_not_of(to_trim) + 1);

		Union::StringANSI::Format("zPreserveOverlays: Get setting [{0}]:{1} from {2}, with value: {3}.\n", section, key, ini.c_str(), str.c_str()).StdPrintLine();

		// Parse ret items into vector, items are separated by two spaces
		size_t pos = 0;
		while (pos < str.length()) {
			size_t next = str.find("  ", pos); // Find two spaces
			if (next == std::string::npos) {
				// Last item
				std::string item = str.substr(pos);
				if (!item.empty()) {
					result.push_back(item);
				}
				break;
			}
			std::string item = str.substr(pos, next - pos);
			if (!item.empty()) {
				result.push_back(item);
			}
			pos = next + 2; // Skip the two spaces
		}
	}
	return result;
}

std::optional<int> GetIntFromParser(const zSTRING& symbol_name)
{
	zCParser* par = zCParser::GetParser();
	if (!par) {
		return {};
	}

	int index = par->GetIndex(symbol_name);
	if (!index) {
		return {};
	}

	zCPar_Symbol* sym = par->GetSymbol(index);
	if (!sym) {
		return {};
	}

	int value = 0;
	sym->GetValue(value, 0);
	Union::StringANSI::Format("zPreserveOverlays: Get value of symbol \"{0}\" from parser: {1}.\n", symbol_name, value).StdPrintLine();
	return value;
}


void __fastcall Impl_Hook_oCNpc_Archive(oCNpc* _this, void* vtable, zCArchiver& archiver);
auto Hook_oCNpc_Archive = Union::CreateHook(SIGNATURE_OF(&oCNpc::Archive), &Impl_Hook_oCNpc_Archive, Union::HookType::Hook_Detours);
void __fastcall Impl_Hook_oCNpc_Archive(oCNpc* _this, void* vtable, zCArchiver& archiver)
{
	if (_this && _this->IsAPlayer() && archiver.InSaveGame()) {
		//SAVING GAME
		//Sometimes 1h overlay e.g. HUMANS_1HST3.MDS is loaded after shield overlay e.g. HUMANS_1HST2SH.MDS
		//Allow reordering, by priority from ini settings

		// Move these overlays if found to the end in this order
		//[ZPRESERVEOVERLAYS]
		//ReorderOverlaysList = HUMANS_RAPIER_ST1.MDS  HUMANS_RAPIER_ST2.MDS  HUMANS_RAPIER_ST3.MDS  HUMANS_1HST1SH.MDS  HUMANS_1HST2SH.MDS  HUMANS_1HST3SH.MDS  HUM_2X2.MDS  ;two spaces separated values

		static std::vector<std::string> specials = GetIniOverlaysList("ZPRESERVEOVERLAYS", "ReorderOverlaysList");
		if (specials.empty()) {
			// setting missinig do nothing
			Hook_oCNpc_Archive(_this, vtable, archiver);
			return;
		}
		zCArray<zSTRING>& activeOverlays = _this->activeOverlays;
		int numOverlays = activeOverlays.GetNum();

		// Temporary storage for found specials (in order they appear in specials vector)
		zCArray<zSTRING> foundSpecials;
		foundSpecials.AllocAbs(specials.size());

		// Mark which overlays are special (for quick lookup)
		zCArray<bool> isSpecial;
		isSpecial.AllocAbs(numOverlays);
		for (int i = 0; i < numOverlays; ++i) {
			isSpecial[i] = false;
		}

		// Find and collect specials in the order defined in specials vector
		for (size_t s = 0; s < specials.size(); ++s) {
			for (int i = 0; i < numOverlays; ++i) {
				if (!isSpecial[i] && activeOverlays[i] == specials[s].c_str()) {
					foundSpecials.Insert(activeOverlays[i]);
					isSpecial[i] = true;
					break; // Found this special, move to next
				}
			}
		}

		// Rebuild array: non-specials first, then specials in order
		int writePos = 0;

		// First pass: copy non-special overlays
		for (int i = 0; i < numOverlays; ++i) {
			if (!isSpecial[i]) {
				if (writePos != i) {
					activeOverlays[writePos] = activeOverlays[i];
				}
				++writePos;
			}
		}

		// Second pass: append found specials in order
		for (int i = 0; i < foundSpecials.GetNum(); ++i) {
			activeOverlays[writePos++] = foundSpecials[i];
		}
	}

	//call original function
    Hook_oCNpc_Archive(_this, vtable, archiver);
}

void __fastcall Impl_Hook_oCNpc_Unarchive(oCNpc* _this, void* vtable, zCArchiver& archiver);
auto Hook_oCNpc_Unarchive = Union::CreateHook(SIGNATURE_OF(&oCNpc::Unarchive), &Impl_Hook_oCNpc_Unarchive, Union::HookType::Hook_Detours);
void __fastcall Impl_Hook_oCNpc_Unarchive(oCNpc* _this, void* vtable, zCArchiver& archiver)
{
	//LOADING GAME
	//call original function
	Hook_oCNpc_Unarchive(_this, vtable, archiver);
	if (_this && _this->IsAPlayer()) {
		oCItem* rh = (oCItem*)_this->GetRightHand();
		oCItem* lh = _this->GetEquippedRangedWeapon();
		int mode = _this->GetWeaponMode();

		//Get symbol values from parser
		static std::optional<int> ITEM_CROSSBOW = GetIntFromParser("ITEM_CROSSBOW");
		static std::optional<int> ITEM_2HD_SWD = GetIntFromParser("ITEM_2HD_SWD");
		static std::optional<int> ITEM_2HD_AXE = GetIntFromParser("ITEM_2HD_AXE");
		if (!ITEM_CROSSBOW || !ITEM_2HD_SWD || !ITEM_2HD_AXE) {
			return;
		}

		if (mode == 4 // Is fight mode
			&& lh && rh // valid pointer
			&& (lh->flags & *ITEM_CROSSBOW) && (lh->munition == 0) && (lh->range > 0) // hack: In G2Alternative, dual weapon is crossbow, with some range and not defined munition. It could maybe sometimes fuck the visuals up in another mod :)
			&& ((rh->flags & *ITEM_2HD_SWD) || (rh->flags & *ITEM_2HD_AXE))) // right hand weapon is two handed sword or axe
		{
			// It is probably dual weapon, on loading weapon stays on back
			// Move visual from ZS_CROSSBOW (hero back) to ZS_LEFTHAND
			zCModel* model = _this->GetModel();
			if (model) {
				zCModelNodeInst* left_hand_node = model->SearchNode("ZS_LEFTHAND");
				zCModelNodeInst* crossbow_node = model->SearchNode("ZS_CROSSBOW");
				if (left_hand_node && crossbow_node) {
					model->SetNodeVisual(left_hand_node, lh->visual, 0);
					model->SetNodeVisual(crossbow_node, nullptr, 0);
				}
			}
		}
	}
}

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

}

void Game_PostLoop()
{

}

void Game_MenuLoop()
{

}

void Game_SaveBegin()
{

}

void Game_SaveEnd()
{

}

void LoadBegin()
{

}

void LoadEnd()
{

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


/*void __fastcall oCGame_MainWorld_Render(Union::Registers& reg);
auto Partial_zCWorld_Render = Union::CreatePartialHook(reinterpret_cast<void*>(zSwitch(0x0063DC76, 0x0066498B, 0x0066BA76, 0x006C87EB)), &oCGame_MainWorld_Render);
void __fastcall oCGame_MainWorld_Render(Union::Registers& reg)
{
	Game_Loop();
}*/

/*void __fastcall zCMenu_Render(zCMenu* self, void* vtable);
auto Hook_zCMenu_Render = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x004D0DA0, 0x004E14E0, 0x004DB270, 0x004DDC20)), &zCMenu_Render, Union::HookType::Hook_Detours);
void __fastcall zCMenu_Render(zCMenu* self, void* vtable)
{
	Hook_zCMenu_Render(self, vtable);
	Game_MenuLoop();
}*/

/*void __fastcall oCGame_WriteSaveGame(oCGame* self, void* vtable, int slot, zBOOL saveGlobals);
auto Hook_oCGame_WriteSaveGame = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063AD80, 0x00661680, 0x006685D0, 0x006C5250)), &oCGame_WriteSaveGame, Union::HookType::Hook_Detours);
void __fastcall oCGame_WriteSaveGame(oCGame* self, void* vtable, int slot, zBOOL saveGlobals)
{
	Game_SaveBegin();
	Hook_oCGame_WriteSaveGame(self, vtable, slot, saveGlobals);
	Game_SaveEnd();
}*/

/*void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, const zSTRING& levelPath);
auto Hook_oCGame_LoadGame = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063C070, 0x00662B20, 0x00669970, 0x006C65A0)), &oCGame_LoadGame, Union::HookType::Hook_Detours);
void __fastcall oCGame_LoadGame(oCGame* self, void* vtable, int slot, const zSTRING& levelPath)
{
	Game_LoadBegin_NewGame();
	Hook_oCGame_LoadGame(self, vtable, slot, levelPath);
	Game_LoadEnd_NewGame();
}*/

/*void __fastcall oCGame_LoadSaveGame(oCGame* self, void* vtable, int slot, zBOOL loadGlobals);
auto Hook_oCGame_LoadSaveGame = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063C2A0, 0x00662D60, 0x00669BA0, 0x006C67D0)), &oCGame_LoadSaveGame, Union::HookType::Hook_Detours);
void __fastcall oCGame_LoadSaveGame(oCGame* self, void* vtable, int slot, zBOOL loadGlobals)
{
	Game_LoadBegin_SaveGame();
	Hook_oCGame_LoadSaveGame(self, vtable, slot, loadGlobals);
	Game_LoadEnd_SaveGame();
}*/

/*void __fastcall oCGame_ChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint);
auto Hook_Game_Load_ChangeLevel = Union::CreateHook(reinterpret_cast<void*>(zSwitch(0x0063CD60, 0x00663950, 0x0066A660, 0x006C7290)), &oCGame_ChangeLevel, Union::HookType::Hook_Detours);
void __fastcall oCGame_ChangeLevel(oCGame* self, void* vtable, const zSTRING& levelpath, const zSTRING& startpoint)
{
	Game_LoadBegin_ChangeLevel();
	Hook_Game_Load_ChangeLevel(self, vtable, levelpath, startpoint);
	Game_LoadEnd_ChangeLevel();
}*/

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
} //namespace GOTHIC_NAMESPACE
