import sys
if "c:/ChibiStudio/workspace/kuroBox/scripts/" not in sys.path:
    sys.path.append("c:/ChibiStudio/workspace/kuroBox/scripts/")

import maya.cmds as mc
import KBB

def mkNode(name='node', node_type='locator', *args, **kwds):
    shape_node = mc.createNode(node_type, name='%sShape' % name, *args, **kwds)
    transform_node = mc.rename(mc.listRelatives(shape_node, parent=True, fullPath=True), name)
    return transform_node, shape_node

def mkCam(name=None, img_plane_imgs=None, *args, **kwds):
    camera_node, camera_shape = mkNode(name, 'camera', *args, **kwds)

    img_plane_node, img_plane_shape = mkNode('%s_imagePlane' % name, 'imagePlane')
    img_plane_node = mc.listRelatives(img_plane_node, shapes=True, fullPath=True)[0]

    # aw100's sensor
    #mc.setAttr('%s.horizontalFilmAperture' % camera_node, 6.17/25.4)
    #mc.setAttr('%s.verticalFilmAperture' % camera_node, 4.55/25.4)

    #mc.setAttr('%s.displayResolution' % camera_node, 1)
    #mc.setAttr('%s.displayFilmGate' % camera_node, 1)

    #mc.setAttr('%s.focalLength' % camera_node, 28.0)
    mc.setAttr('%s.focalLength' % camera_node, 50.0)

    mc.setAttr('%s.alphaGain' % img_plane_node, 0.85)
    mc.setAttr('%s.useFrameExtension' % img_plane_node, 1)

    if img_plane_imgs:
        mc.setAttr( "%s.imageName" % img_plane_node, img_plane_imgs, type="string")
    mc.connectAttr('%s.message' % img_plane_node, '%s.imagePlane[0]' % camera_node)
    mc.expression(s="%s.frameExtension=frame+%s.frameOffset"%(img_plane_node,img_plane_node))

    annotate_node = mc.annotate(name, text="tc=...", point=(0, -3, -10))
    annotate_node = mc.rename(mc.listRelatives(annotate_node, parent=True, fullPath=True), "%s_timecode"%name)
    group_node = mc.group(camera_node, img_plane_node, annotate_node, name="%s_group"%name)

    yaw_node, yaw_shape = mkNode("%s_yaw"%name,"locator")
    pitch_node, pitch_shape = mkNode("%s_pitch"%name,"locator")
    roll_node, roll_shape = mkNode("%s_roll"%name,"locator")
    kb_node, kb_shape = mkNode("%s_kuroBox"%name,"locator")

    yaw_node = mc.ls(mc.parent(yaw_node, kb_node, relative=True)[0], long=True)[0]
    pitch_node = mc.ls(mc.parent(pitch_node, yaw_node, relative=True)[0], long=True)[0]
    roll_node = mc.ls(mc.parent(roll_node, pitch_node, relative=True)[0], long=True)[0]
    group_node = mc.ls(mc.parent(group_node, roll_node, relative=True)[0], long=True)[0]

    mc.addAttr(kb_node, longName='tc_h', attributeType='byte')
    mc.addAttr(kb_node, longName='tc_m', attributeType='byte')
    mc.addAttr(kb_node, longName='tc_s', attributeType='byte')
    mc.addAttr(kb_node, longName='tc_f', attributeType='byte')

    script="""string $tc = "";
string $h = $$TC$$.tc_h;
string $m = $$TC$$.tc_m;
string $s = $$TC$$.tc_s;
string $f = $$TC$$.tc_f;
if (size($h)==1) $h = "0" + $h;
if (size($m)==1) $m = "0" + $m;
if (size($s)==1) $s = "0" + $s;
if (size($f)==1) $f = "0" + $f;
$tc = "tc=" + $h + ":" + $m + ":" + $s + "." + $f;
setAttr cam_timecodeShape.text -type \"string\" $tc;
""".replace("$$TC$$","cam_kuroBox")
    mc.expression(name="%s_tc_update"%annotate_node, object=annotate_node, string=script)

    return kb_node, yaw_node, pitch_node, roll_node

def is_sane(v):
    return v<=360.0 and v>=-360.0

def animateCam(kb_node, yaw_node, pitch_node, roll_node, kbb_file):
    mc.currentUnit(time="film")
    kbb = KBB.KBB_factory(kbb_file)
    kbb.set_index(0)
    first_tc_idx = 0
    while kbb.read_next():
        if kbb.ltc.hours != 0 or kbb.ltc.minutes != 0 or kbb.ltc.seconds != 0 or kbb.ltc.frames != 0:
            break
        first_tc_idx += 1
    print "First TC index:", first_tc_idx, kbb.ltc
    skip_item = None
    frame = 1
    while kbb.read_next():
        if skip_item != kbb.ltc.frames:
            print kbb.header.msg_num, kbb.ltc, frame
            skip_item = kbb.ltc.frames
            mc.setKeyframe(kb_node, attribute='tc_h', time=frame, value=kbb.ltc.hours)
            mc.setKeyframe(kb_node, attribute='tc_m', time=frame, value=kbb.ltc.minutes)
            mc.setKeyframe(kb_node, attribute='tc_s', time=frame, value=kbb.ltc.seconds)
            mc.setKeyframe(kb_node, attribute='tc_f', time=frame, value=kbb.ltc.frames)
            if is_sane(kbb.vnav.yaw): mc.setKeyframe(yaw_node, attribute='ry', time=frame, value=-kbb.vnav.yaw)
            if is_sane(kbb.vnav.pitch): mc.setKeyframe(pitch_node, attribute='rx', time=frame, value=kbb.vnav.pitch)
            if is_sane(kbb.vnav.roll): mc.setKeyframe(roll_node, attribute='rz', time=frame, value=180.0-kbb.vnav.roll)
            frame += 1

    # get rid of the full-path using mc.ls()
    mc.filterCurve("%s_rotateY"%mc.ls(yaw_node)[0])
    mc.filterCurve("%s_rotateX"%mc.ls(pitch_node)[0])
    mc.filterCurve("%s_rotateZ"%mc.ls(roll_node)[0])
    mc.keyTangent("%s_rotateY"%mc.ls(yaw_node)[0], outTangentType='linear')
    mc.keyTangent("%s_rotateX"%mc.ls(pitch_node)[0], outTangentType='linear')
    mc.keyTangent("%s_rotateZ"%mc.ls(roll_node)[0], outTangentType='linear')

    mc.playbackOptions(minTime=1,maxTime=frame)


def mkScene():
    skydome = mc.polySphere(name="skydome", r=1000)
    shader = mc.shadingNode('blinn', asShader=True)
    checker = mc.shadingNode("file", asTexture=True)
    mc.setAttr("%s.fileTextureName"%checker,"c:/ChibiStudio/workspace/kuroBox/scripts/UV.png",type="string")
    mc.setAttr("%s.repeatUV"%checker, 5, 5)
    mc.connectAttr("%s.outColor"%checker, "%s.color"%shader, force=True)
    mc.select(skydome)
    mc.hyperShade(assign=shader)

"""
mc.file( force=True, new=True )
maya_tools.mkScene()
kb_node, yaw_node, pitch_node, roll_node=maya_tools.mkCam("cam","C:/ChibiStudio/workspace/KUROBOX_TEST_DATA/DSCN7168/DSCN7168.00001.jpg")
maya_tools.animateCam(kb_node, yaw_node, pitch_node, roll_node,"C:/ChibiStudio/workspace/KUROBOX_TEST_DATA/DSCN7168/KURO0028.KBB")


top=maya_tools.mkCam("cam","C:/ChibiStudio/workspace/KUROBOX_TEST_DATA/DSCN7165/DSCN7165.00001.jpg")
maya_tools.animateCam(top,"C:/ChibiStudio/workspace/KUROBOX_TEST_DATA/KURO0025.KBB")
print top
skydome = mc.polySphere(name="skydome", r=100)
shader = mc.shadingNode('blinn', asShader=True)
checker = mc.shadingNode("file", asTexture=True)
mc.setAttr("%s.fileTextureName"%checker,"c:/ChibiStudio/workspace/kuroBox/scripts/UV.png",type="string")
mc.connectAttr("%s.outColor"%checker, "%s.color"%shader, force=True)
mc.select(skydome)
mc.sets(e=True, forceElement=shader)
mc.hyperShade(assign=shader)
"""