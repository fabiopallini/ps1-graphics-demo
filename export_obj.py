import bpy

export_name = "c1run"
export_path = "/home/fabio/mData/projects/ps1-dev/psyq/projects/demo/data/chars/char1/"

frames_to_export = [0, 10, 20, 30, 40]

export_counter = 0

for frame in frames_to_export:
    bpy.context.scene.frame_set(frame)
    
    '''
    selected_object = bpy.context.view_layer.objects.active
    if not selected_object:
        print("please select an object")
        break
    export_name = selected_object.name  # Nome dell'oggetto
    '''

    file_name = f"{export_name}{export_counter}.obj"
    full_path = export_path + file_name
    
    bpy.ops.export_scene.obj(
        filepath=full_path,
        use_selection=True,
        use_mesh_modifiers=True,
        use_uvs=True,
        axis_forward='Z',
        axis_up='-Y',
        use_materials=False,
        use_edges=False,
        use_normals=False,
        use_vertex_groups=False,
        use_smooth_groups=False,
        use_smooth_groups_bitflags=False,
        use_triangles=False,
        use_nurbs=False,
        group_by_object=False,
        group_by_material=False
    )
    
    print(f"exported file: {file_name}")
    
    export_counter += 1

bpy.context.scene.frame_set(0)
print("export success!")

