# zPreserveOverlays

Union plugin for Gothic II Notr.
Created and usable for Gothic II Alternative mod.

It implements two main features.

## Reordering active overlays

Reorders oCNpc::activeOverlays for hero instance, when saving game (oCNpc::Archive), so the more precedent overlay is next time loaded last.
It could maybe somehow cause issues, when someone has pointer for overlay string. Thats why zSTRING pointers in zCArray are reordered, and not created anew.

Define ReorderOverlayList in mod.ini/Gothic.ini/SystemPack.ini

```
[ZPRESERVEOVERLAYS]
ReorderOverlaysList = HUMANS_RAPIER_ST1.MDS  HUMANS_RAPIER_ST2.MDS  HUMANS_RAPIER_ST3.MDS  HUMANS_1HST1SH.MDS  HUMANS_1HST2SH.MDS  HUMANS_1HST3SH.MDS  HUM_2X2.MDS  ;two spaces separated values
```

Written overlays will be ordered last if found, in the same order like in this list. So shield animation will have precedence over 1h/2h animation. But 2x2 (dual weapons) will have precedence even over shields animation.

## Hack for putting second sword to hand

When saving game with 2x2 animation (dual weapons) in fight mode, and then loading the game, second weapon is stuck on back, and put to hand when sheating weapon :D.
Detecting this case, by looking on equipped weapons, and mode at the end of oCNpc::Unarchive.
Detection is a bit clumsy, it looks for ITEM_CROSSBOW equipped range item, that has no munition, but has range.

I played a bit of game with it, improving it as I played...
Use at your own risk :)

Based on new Union framework template: https://github.com/Patrix9999/union-plugin-template/
