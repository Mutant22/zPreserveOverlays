// This file added in headers queue
// File: "Sources.h"
#include "resource.h"

namespace GOTHIC_ENGINE {

static zCArchiver* CreateArchiverWrite(zSTRING path)
{
    static bool saveGameToAnisValid = false;
    static bool saveGameToAnsi;

    if (!saveGameToAnisValid)
    {
        saveGameToAnisValid = true;
        COption& unionOpt = Union.GetUnionOption();
        unionOpt.Read(saveGameToAnsi, "GAME", "SaveGameToANSI");
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
    zCArchiver* arc = CreateArchiverWrite(GetSavePath());

    //
    arc->WriteInt("SIZE", overlays.GetNum());

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
        player->ApplyOverlay(overlay);
    }
}

OverlayHolder g_overlays;


  void Game_Entry() {
  }
  
  void Game_Init() {
  }

  void Game_Exit() {
  }

  void Game_PreLoop() {
  }

  void Game_Loop() {
      //update overlays
      g_overlays.OnUpdate();
  }

  void Game_PostLoop() {
  }

  void Game_MenuLoop() {
  }

  // Information about current saving or loading world
  TSaveLoadGameInfo& SaveLoadGameInfo = UnionCore::SaveLoadGameInfo;

  void Game_SaveBegin() {
      //save overlays
      g_overlays.OnSave();
  }

  void Game_SaveEnd() {
  }

  void LoadBegin() {
  }

  void LoadEnd() {
      //restore overlays
      g_overlays.OnLoad();
  }

  void Game_LoadBegin_NewGame() {
    LoadBegin();
  }

  void Game_LoadEnd_NewGame() {
    LoadEnd();
  }

  void Game_LoadBegin_SaveGame() {
    LoadBegin();
  }

  void Game_LoadEnd_SaveGame() {
    LoadEnd();
  }

  void Game_LoadBegin_ChangeLevel() {
    LoadBegin();
  }

  void Game_LoadEnd_ChangeLevel() {
    LoadEnd();
  }

  void Game_LoadBegin_Trigger() {
  }
  
  void Game_LoadEnd_Trigger() {
  }
  
  void Game_Pause() {
  }
  
  void Game_Unpause() {
  }
  
  void Game_DefineExternals() {
  }

  void Game_ApplyOptions() {
  }

  /*
  Functions call order on Game initialization:
    - Game_Entry           * Gothic entry point
    - Game_DefineExternals * Define external script functions
    - Game_Init            * After DAT files init
  
  Functions call order on Change level:
    - Game_LoadBegin_Trigger     * Entry in trigger
    - Game_LoadEnd_Trigger       *
    - Game_Loop                  * Frame call window
    - Game_LoadBegin_ChangeLevel * Load begin
    - Game_SaveBegin             * Save previous level information
    - Game_SaveEnd               *
    - Game_LoadEnd_ChangeLevel   *
  
  Functions call order on Save game:
    - Game_Pause     * Open menu
    - Game_Unpause   * Click on save
    - Game_Loop      * Frame call window
    - Game_SaveBegin * Save begin
    - Game_SaveEnd   *
  
  Functions call order on Load game:
    - Game_Pause              * Open menu
    - Game_Unpause            * Click on load
    - Game_LoadBegin_SaveGame * Load begin
    - Game_LoadEnd_SaveGame   *
  */

#define AppDefault True
  CApplication* lpApplication = !CHECK_THIS_ENGINE ? Null : CApplication::CreateRefApplication(
    Enabled( AppDefault ) Game_Entry,
    Enabled( AppDefault ) Game_Init,
    Enabled( AppDefault ) Game_Exit,
    Enabled( AppDefault ) Game_PreLoop,
    Enabled( AppDefault ) Game_Loop,
    Enabled( AppDefault ) Game_PostLoop,
    Enabled( AppDefault ) Game_MenuLoop,
    Enabled( AppDefault ) Game_SaveBegin,
    Enabled( AppDefault ) Game_SaveEnd,
    Enabled( AppDefault ) Game_LoadBegin_NewGame,
    Enabled( AppDefault ) Game_LoadEnd_NewGame,
    Enabled( AppDefault ) Game_LoadBegin_SaveGame,
    Enabled( AppDefault ) Game_LoadEnd_SaveGame,
    Enabled( AppDefault ) Game_LoadBegin_ChangeLevel,
    Enabled( AppDefault ) Game_LoadEnd_ChangeLevel,
    Enabled( AppDefault ) Game_LoadBegin_Trigger,
    Enabled( AppDefault ) Game_LoadEnd_Trigger,
    Enabled( AppDefault ) Game_Pause,
    Enabled( AppDefault ) Game_Unpause,
    Enabled( AppDefault ) Game_DefineExternals,
    Enabled( AppDefault ) Game_ApplyOptions
  );
}