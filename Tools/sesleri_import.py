# Art/Audio/*.wav -> /Game/Sallanti/Audio. Headless calisir; editor KAPALI olmali.
# Mevcut asset'in ustune import YAPMAZ (import ayarlari eskide kalir): once siler, sonra import eder.
# Gercek ses dosyalari gelince ayni isimle Art/Audio'ya koyup bu betigi tekrar calistirmak yeter.
import os
import unreal

EAL = unreal.EditorAssetLibrary
SRC = os.path.normpath(os.path.join(os.path.dirname(__file__), "..", "Art", "Audio"))
DST = "/Game/Sallanti/Audio"
LOOPS = {"SFX_Gicirti_Dongu"}

files = sorted(f for f in os.listdir(SRC) if f.lower().endswith(".wav"))
if not files:
    raise RuntimeError("SL_SFX kaynak yok: " + SRC)

tasks = []
for f in files:
    name = os.path.splitext(f)[0]
    path = DST + "/" + name
    if EAL.does_asset_exist(path) and not EAL.delete_asset(path):
        raise RuntimeError("SL_SFX silinemedi " + path)
    t = unreal.AssetImportTask()
    t.filename = os.path.join(SRC, f)
    t.destination_path = DST
    t.destination_name = name
    t.automated = True
    t.save = True
    t.replace_existing = False
    tasks.append(t)

unreal.AssetToolsHelpers.get_asset_tools().import_asset_tasks(tasks)

ok = 0
for f in files:
    name = os.path.splitext(f)[0]
    path = DST + "/" + name
    snd = EAL.load_asset(path)
    if not isinstance(snd, unreal.SoundWave):
        unreal.log_error("SL_SFX import BASARISIZ " + path)
        continue
    if name in LOOPS:
        snd.set_editor_property("looping", True)
        if not EAL.save_asset(path, only_if_is_dirty=False):
            raise RuntimeError("SL_SFX kaydedilemedi " + path)
    unreal.log("SL_SFX %s sure=%.2f dongu=%s" % (name, snd.get_editor_property("duration"), snd.get_editor_property("looping")))
    ok += 1
unreal.log("SL_SFX DONE %d/%d" % (ok, len(files)))
