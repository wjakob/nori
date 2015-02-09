
bl_info = {
    "name": "Export Nori scenes format",
    "author": "Adrien Gruson",
    "version": (0, 1),
    "blender": (2, 5, 7),
    "location": "File > Export > Nori exporter (.xml)",
    "description": "Export Nori scenes format (.xml)",
    "warning": "",
    "wiki_url": "",
    "tracker_url": "",
    "category": "Import-Export"}

import bpy, os, math, shutil
from xml.dom.minidom import Document

# Main class exporter
class NoriWritter:
    def verbose(self,text):
        print(text)
    
    def __init__(self, context, filepath):
        self.context = context
        self.filepath = filepath
        self.workingDir = os.path.dirname(self.filepath)

    ######################
    # tools private methods
    # (xml format)
    ######################
    def __createElement(self, name, attr):
        el = self.doc.createElement(name)
        for k,v in attr.items():
            el.setAttribute(k,v)
        return el

    def __createEntry(self, t, name, value):
        return self.__createElement(t,{"name":name,"value":value}) 

    def __createVector(self, t, vec):
        return self.__createElement(t, {"value": "%f %f %f" % (vec[0],vec[1],vec[2])})

    def __createTransform(self, mat, el = None):
        transform = self.__createElement("transform",{"name":"toWorld"})
        if(el):
            transform.appendChild(el)
        value = ""
        for j in range(4):
            for i in range(4):
                value += str(mat[j][i])+","
        transform.appendChild(self.__createElement("matrix",{"value":value[:-1]}))
        return transform

    def write(self, exportLight, nbSamples):
        """Main method to write the blender scene into Nori format
        It will export as follows:
         1) write integrator configuration
         2) write samples information (number, distribution)
         3) export one camera
         4) export all light sources
         5) export all meshes"""
        
        # create xml document
        self.doc = Document()
        self.scene = self.doc.createElement("scene")
        self.doc.appendChild(self.scene)
        
        ######################
        # 1) write integrator configuration
        ######################
        if(not exportLight):
            self.scene.appendChild(self.__createElement("integrator", {"type" : "av" }))
        else:
            self.scene.appendChild(self.__createElement("integrator", {"type" : "path_mis" }))

        ######################
        # 2) write the number of samples
        # and which distribution we will use
        ######################
        sampler = self.__createElement("sampler", {"type" : "independent" })
        sampler.appendChild(self.__createElement("integer", {"name":"sampleCount", "value":str(nbSamples)}))
        self.scene.appendChild(sampler)

        ######################
        # 3) export one camera
        ######################
        # note that we only support one camera
        cameras = [cam for cam in self.context.scene.objects
                       if cam.type in {'CAMERA'}]
        if(len(cameras) == 0):
            self.verbose("WARN: No camera to export")
        else:
            if(len(cameras) > 1):
                self.verbose("WARN: Does not handle multiple camera, only export the first one")
            self.scene.appendChild(self.write_camera(cameras[0])) # export the first one

        ######################
        # 4) export all light sources
        ######################
        if(exportLight):
            sources = [obj for obj in self.context.scene.objects
                          if obj.type in {'LAMP'}]
            for source in sources:
                if(source.data.type == "POINT"):
                    pointLight = self.__createElement("emitter", {"type" : "point" })
                    pos = source.location
                    pointLight.appendChild(self.__createEntry("point", "position", "%f,%f,%f"%(pos.x,pos.y,pos.z)))
                    self.scene.appendChild(pointLight)
                else:
                    self.verbose("WARN: Light source type (%s) is not supported" % source.data.type)
        
        ######################
        # 5) export all meshes
        ######################
        # create the directory for store the meshes
        if not os.path.exists(self.workingDir+"/meshes"):
                os.makedirs(self.workingDir+"/meshes")
        
        # export all of them
        meshes = [obj for obj in self.context.scene.objects
                      if obj.type in {'MESH', 'EMPTY'}
                      and obj.parent is None]
        for mesh in meshes:
            self.write_mesh(mesh)
        
        ######################
        # 6) write the xml file
        ######################
        self.doc.writexml(open(self.filepath, "w"), "", "\t","\n")

    def write_camera(self, cam):
        """convert the selected camera (cam) into xml format"""
        camera = self.__createElement("camera",{"type":"perspective"})
        camera.appendChild(self.__createEntry("float","fov",str(cam.data.angle*180/math.pi)))
        camera.appendChild(self.__createEntry("float","nearClip",str(cam.data.clip_start)))
        camera.appendChild(self.__createEntry("float","farClip",str(cam.data.clip_end)))
        percent = self.context.scene.render.resolution_percentage/100.0
        camera.appendChild(self.__createEntry("integer","width",str(int(self.context.scene.render.resolution_x*percent))))
        camera.appendChild(self.__createEntry("integer","height",str(int(self.context.scene.render.resolution_y*percent))))
        trans = self.__createTransform(cam.matrix_world, self.__createVector("scale",(1,1,-1)))
        camera.appendChild(trans)
        return camera

    ######################
    # meshes related methods
    ######################
    def __createMeshEntry(self, filename, matrix):
        meshElement = self.__createElement("mesh", {"type" : "obj"})
        meshElement.appendChild(self.__createElement("string", {"name":"filename","value":"meshes/"+filename}))
        meshElement.appendChild(self.__createTransform(matrix))
        return meshElement

    def __createBSDFEntry(self, slot):
        """method responsible to the auto-conversion
        between Blender internal BSDF (not Cycles!) and Nori BSDF
        
        For more advanced implementation: 
        http://tinyurl.com/nnhxwuh
        """
        if slot.material.raytrace_mirror.use:
            return self.__createElement("bsdf", {"type":"mirror"})
        else:
            bsdfElement = self.__createElement("bsdf", {"type":"diffuse"})
            c = slot.material.diffuse_color
            bsdfElement.appendChild(self.__createEntry("color", "albedo","%f,%f,%f" %(c[0],c[1],c[2])))
            return bsdfElement

    def write_mesh(self,mesh):
        children_mesh = [obj for obj in mesh.children
                      if obj.type in {'MESH', 'EMPTY'}]

        for child in children_mesh:
            self.write_mesh(child)
        
        if mesh.type == 'MESH':
            for meshEntry in self.write_mesh_objs(mesh):
                self.scene.appendChild(meshEntry)
    
    def write_face(self, prevMesh, fileObj, exportUV, exportNormal, idMat = -1):
        for poly in prevMesh.polygons:
            
            if((idMat != -1) and (poly.material_index == idMat)):
                
                # Check if it's not a cube
                vertFaces = [poly.vertices[:]]
                if len(vertFaces[0]) == 4:
                    vertFaces = [(vertFaces[0][0],vertFaces[0][1], vertFaces[0][2]),
                                 (vertFaces[0][2],vertFaces[0][3], vertFaces[0][0])]
                elif len(vertFaces[0]) == 3:
                    pass # Nothing to do
                else:
                    raise "Exception: Difficult poly, abord"
                
                for vert in vertFaces:
                    face = "f"
                    
                    
                    # Order checking
                    if(not exportNormal):
                        ac = prevMesh.vertices[vert[2]].co - prevMesh.vertices[vert[0]].co
                        ab = prevMesh.vertices[vert[1]].co - prevMesh.vertices[vert[0]].co
                        norm = ab.cross(ac)
                        norm.normalize()
                        
                        # Need to inverse order
                        if(norm.dot(poly.normal) < 0.0):
                            print("Normal flip: "+str(poly.normal)+" != "+str(norm))
                            vert = (vert[2],vert[1],vert[0])
                            
                    for idVert in vert:
                        face += " "+str(idVert+1)
                        
                        # Nothing to do for the export
                        if((not exportUV) and (not exportNormal)):
                            continue
                        
                        if(exportUV):
                            face += "/"+str(idVert+1)
                        else:
                            face += "/"
                        
                        if(exportNormal):
                            face += "/"+str(idVert+1)
                        
                    fileObj.write(face+"\n")
    
    def write_mesh_objs(self, mesh):
        # convert the shape by apply all modifier
        prevMesh = mesh.to_mesh(bpy.context.scene, True, "PREVIEW")
        
        # get useful information of the shape
        exportNormal = (prevMesh.polygons[0].use_smooth)
        exportUV = exportNormal and (prevMesh.uv_layers.active != None)
        haveMaterial = (len(mesh.material_slots) != 0 and mesh.material_slots[0].name != '')
        
        # export obj file base (vertex pos, normal and uv)
        # but not the face data
        fileObjPath = mesh.name+".obj" 
        fileObj = open(self.workingDir+"/meshes/"+fileObjPath, "w")

        # write all vertex informations
        for vert in prevMesh.vertices:
            fileObj.write('v %f %f %f\n' % (vert.co.x, vert.co.y, vert.co.z))
        
        if exportUV:
            uvlist =  prevMesh.uv_layers.active.data
            for uvvert in uvlist:
                fileObj.write('vt %f %f \n' % (uvvert.uv.x, uvvert.uv.y))
        
        if exportNormal:
            for vert in prevMesh.vertices:
                fileObj.write('vn %f %f %f\n' % (vert.normal.x, vert.normal.y, vert.normal.z))
        
        # write all polygones (faces)
        listMeshXML = []
        if(not haveMaterial):
            self.write_face(prevMesh, fileObj, exportUV, exportNormal)
                
            # add default BSDF
            meshElement = self.__createMeshEntry(fileObjPath, mesh.matrix_world)
            bsdfElement = self.__createElement("bsdf", {"type":"diffuse"})
            bsdfElement.appendChild(self.__createEntry("color", "albedo", "0.75,0.75,0.75"))
            meshElement.appendChild(bsdfElement)
            listMeshXML = [meshElement]
        else:
            fileObj.close()
            for id_mat in range(len(mesh.material_slots)):
                slot = mesh.material_slots[id_mat]
                self.verbose("MESH: "+mesh.name+" BSDF: "+slot.name)
                
                # we create an new obj file and concatenate data files
                fileObjMatPath = mesh.name+"_"+slot.name+".obj" 
                fileObjMat = open(self.workingDir+"/meshes/"+fileObjMatPath,"w")
                shutil.copyfileobj(open(self.workingDir+"/meshes/"+fileObjPath,"r"), fileObjMat)
                
                # we write all face material specific
                self.write_face(prevMesh, fileObjMat, exportUV, exportNormal, id_mat)
                
                # We create xml related entry
                meshElement = self.__createMeshEntry(fileObjMatPath, mesh.matrix_world)
                meshElement.appendChild(self.__createBSDFEntry(slot))
                listMeshXML.append(meshElement)
                
                fileObjMat.close()
            
            # Clean temporal obj file
            os.remove(self.workingDir+"/meshes/"+fileObjPath)
                
        # free the memory
        bpy.data.meshes.remove(prevMesh)

        
        return listMeshXML

######################
# blender code
######################
from bpy.props import StringProperty, IntProperty, BoolProperty
from bpy_extras.io_utils import ExportHelper

class NoriExporter(bpy.types.Operator, ExportHelper):
    """Export a blender scene into Nori scene format"""
    
    # add to menu
    bl_idname = "export.nori"
    bl_label = "Export Nori scene"

    # filtering file names
    filename_ext = ".xml"
    filter_glob = StringProperty(default="*.xml", options={'HIDDEN'})
    
    ###################
    # other options
    ###################
    
    export_light = BoolProperty(
                    name="Export light",
                    description="Export light to Nori",
                    default=True)
    
    nb_samples = IntProperty(name="Numbers of camera rays",
                    description="Number of camera ray",
                    default=32)
    
    def execute(self, context):
        nori = NoriWritter(context, self.filepath)
        nori.write(self.export_light, self.nb_samples)
        return {'FINISHED'}
    
    def invoke(self, context, event):
        #self.frame_start = context.scene.frame_start
        #self.frame_end = context.scene.frame_end

        wm = context.window_manager
        wm.fileselect_add(self)
        return {'RUNNING_MODAL'}


def menu_export(self, context):
    import os
    default_path = os.path.splitext(bpy.data.filepath)[0] + ".xml"
    self.layout.operator(NoriExporter.bl_idname, text="Export Nori scenes...").filepath = default_path


# Register Nori exporter inside blender
def register():
    bpy.utils.register_module(__name__)
    bpy.types.INFO_MT_file_export.append(menu_export)

def unregister():
    bpy.utils.unregister_module(__name__)
    bpy.types.INFO_MT_file_export.remove(menu_export)

if __name__ == "__main__":
    register()
