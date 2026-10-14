# Gun 0: test/ana seviye + karton ve hayalet materyalleri. Headless calisir.
# Tekrar calistirilirsa var olani EZMEZ (atlar).
import unreal

EAL = unreal.EditorAssetLibrary
MEL = unreal.MaterialEditingLibrary
tools = unreal.AssetToolsHelpers.get_asset_tools()
results = []

def fail(msg):
    unreal.log_error("SL_SETUP FAIL: " + msg)
    raise RuntimeError(msg)

def make_material(name, path, build):
    full = path + "/" + name
    if EAL.does_asset_exist(full):
        results.append("SKIP " + full); return
    m = tools.create_asset(name, path, unreal.Material, unreal.MaterialFactoryNew())
    if not m: fail("create " + full)
    build(m)
    MEL.recompile_material(m)
    if not EAL.save_asset(full, only_if_is_dirty=False): fail("save " + full)
    results.append("OK " + full)

def build_cardboard(m):
    tint = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -400, 0)
    tint.set_editor_property("parameter_name", "Tint")
    tint.set_editor_property("default_value", unreal.LinearColor(0.62, 0.45, 0.28, 1))
    MEL.connect_material_property(tint, "", unreal.MaterialProperty.MP_BASE_COLOR)
    r = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 200)
    r.set_editor_property("parameter_name", "Roughness"); r.set_editor_property("default_value", 0.9)
    MEL.connect_material_property(r, "", unreal.MaterialProperty.MP_ROUGHNESS)

def build_ghost(m):
    m.set_editor_property("blend_mode", unreal.BlendMode.BLEND_TRANSLUCENT)
    m.set_editor_property("shading_model", unreal.MaterialShadingModel.MSM_UNLIT)
    c = MEL.create_material_expression(m, unreal.MaterialExpressionVectorParameter, -400, 0)
    c.set_editor_property("parameter_name", "Color")
    c.set_editor_property("default_value", unreal.LinearColor(0.2, 0.9, 0.3, 1))
    MEL.connect_material_property(c, "", unreal.MaterialProperty.MP_EMISSIVE_COLOR)
    o = MEL.create_material_expression(m, unreal.MaterialExpressionScalarParameter, -400, 200)
    o.set_editor_property("parameter_name", "Opacity"); o.set_editor_property("default_value", 0.45)
    MEL.connect_material_property(o, "", unreal.MaterialProperty.MP_OPACITY)

make_material("M_Cardboard", "/Game/Sallanti/Materials", build_cardboard)
make_material("M_Ghost", "/Game/Sallanti/Materials", build_ghost)

les = unreal.get_editor_subsystem(unreal.LevelEditorSubsystem)
ues = unreal.get_editor_subsystem(unreal.UnrealEditorSubsystem)
eas = unreal.get_editor_subsystem(unreal.EditorActorSubsystem)

def make_level(full):
    if EAL.does_asset_exist(full):
        results.append("SKIP " + full); return
    if not les.new_level(full): fail("new_level " + full)
    world = ues.get_editor_world()
    if world is None: fail("no editor world after new_level")
    sun = eas.spawn_actor_from_class(unreal.DirectionalLight, unreal.Vector(0, 0, 1000), unreal.Rotator(0, -50, -30))
    if not sun: fail("spawn sun")
    sun.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sun.light_component.set_editor_property("atmosphere_sun_light", True)
    sky = eas.spawn_actor_from_class(unreal.SkyLight, unreal.Vector(0, 0, 800))
    sky.light_component.set_editor_property("mobility", unreal.ComponentMobility.MOVABLE)
    sky.light_component.set_editor_property("real_time_capture", True)
    eas.spawn_actor_from_class(unreal.SkyAtmosphere, unreal.Vector(0, 0, 0))
    eas.spawn_actor_from_class(unreal.ExponentialHeightFog, unreal.Vector(0, 0, -500))
    if not les.save_current_level(): fail("save " + full)
    results.append("OK " + full)

make_level("/Game/Sallanti/Maps/L_Ana")
make_level("/Game/Sallanti/Maps/L_Test")

for r in results: unreal.log("SL_SETUP " + r)
unreal.log("SL_SETUP DONE")
