
// This file added in headers queue
// File: "Headers.h"

namespace GOTHIC_ENGINE {
	class OverlayHolder {
	public:
		void Clear() { overlays.Clear(); }
		void Add(const zSTRING& o) { overlays.InsertEnd(o); }
		void OnSave();
		void OnLoad();
		void OnUpdate();
		void Apply();

	private:
		Array<zSTRING> overlays;
	};
}
