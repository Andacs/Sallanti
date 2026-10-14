# M_Cardboard (ve M_Ghost) instanced static mesh ile kullanilabilsin: bayrak yoksa oyun karolari
# varsayilan gri malzemeyle cizer (14 Eki: "missing usage flag InstancedStaticMeshes").
import unreal
EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
for path in ["/Game/Sallanti/Materials/M_Cardboard", "/Game/Sallanti/Materials/M_Ghost"]:
    m = EAL.load_asset(path)
    if not m:
        raise RuntimeError("SL_MAT yuklenemedi " + path)
    before = m.get_editor_property("used_with_instanced_static_meshes")
    m.set_editor_property("used_with_instanced_static_meshes", True)
    MEL.recompile_material(m)
    if not EAL.save_asset(path, only_if_is_dirty=False):
        raise RuntimeError("SL_MAT kaydedilemedi " + path)
    after = EAL.load_asset(path).get_editor_property("used_with_instanced_static_meshes")
    unreal.log("SL_MAT %s once=%s sonra=%s" % (path, before, after))
unreal.log("SL_MAT DONE")
