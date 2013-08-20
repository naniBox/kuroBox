#!/usr/bin/env python2
"""
    kuroBox / naniBox
    Copyright (c) 2013
    david morris-oliveros // naniBox.com

    This file is part of kuroBox / naniBox.

    kuroBox / naniBox is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 3 of the License, or
    (at your option) any later version.

    kuroBox / naniBox is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

"""

import sys
if "c:/ChibiStudio/workspace/kuroBox/scripts/" not in sys.path:
    sys.path.append("c:/ChibiStudio/workspace/kuroBox/scripts/")

import maya.cmds as mc
import KBB

def mkNode(name="node", node_type="locator", *args, **kwds):
    shape_node = mc.createNode(node_type, name="%sShape" % name, *args, **kwds)
    transform_node = mc.rename(mc.listRelatives(shape_node, parent=True, fullPath=True), name)
    return transform_node

def mkCam(name=None, img_plane_imgs=None, *args, **kwds):
    camera_node = mkNode("%s_cam"%name, "camera", *args, **kwds)
    img_plane_node = mkNode("%s_imagePlane"%name, "imagePlane")
    img_plane_node = mc.listRelatives(img_plane_node, shapes=True, fullPath=True)[0]
    annotate_node = mc.annotate("%s_cam"%name, text="tc=...", point=(0, -3, -10))
    annotate_node = mc.rename(mc.listRelatives(annotate_node, parent=True, fullPath=True), "%s_timecode"%name)

    if img_plane_imgs:
        mc.setAttr( "%s.imageName" % img_plane_node, img_plane_imgs, type="string")
    mc.connectAttr("%s.message" % img_plane_node, "%s.imagePlane[0]" % camera_node)
    # you'd think that this was the default, to actually use frameOffset, but nooooooo....
    mc.expression(s="%s.frameExtension=frame+%s.frameOffset"%(img_plane_node,img_plane_node))

    mc.setAttr("%s.rotateOrder"%camera_node, 3)
    mc.setAttr("%s.focalLength"%camera_node, 28.0)
    mc.setAttr("%s.alphaGain"%img_plane_node, 0.85)
    mc.setAttr("%s.useFrameExtension"%img_plane_node, 1)

    mc.addAttr(camera_node, longName="tc_h", attributeType="byte")
    mc.addAttr(camera_node, longName="tc_m", attributeType="byte")
    mc.addAttr(camera_node, longName="tc_s", attributeType="byte")
    mc.addAttr(camera_node, longName="tc_f", attributeType="byte")
    mc.parent(annotate_node, img_plane_node, camera_node, relative=True)

    script="""string $tc = "";
string $h = $$NAME$$_cam.tc_h;
string $m = $$NAME$$_cam.tc_m;
string $s = $$NAME$$_cam.tc_s;
string $f = $$NAME$$_cam.tc_f;
if (size($h)==1) $h = "0" + $h;
if (size($m)==1) $m = "0" + $m;
if (size($s)==1) $s = "0" + $s;
if (size($f)==1) $f = "0" + $f;
$tc = "tc=" + $h + ":" + $m + ":" + $s + "." + $f;
setAttr $$NAME$$_timecodeShape.text -type \"string\" $tc;
""".replace("$$NAME$$",name)
    mc.expression(name="%s_tc_update"%annotate_node, object=annotate_node, string=script)

    mc.select(camera_node)

    return camera_node

def is_sane(v):
    #return True
    return v<=360.0 and v>=-360.0

def animateCam(kb_node, kbb_file, frame_rate=30):
    if frame_rate == 24:
        mc.currentUnit(time="film")
    elif frame_rate == 25:
        mc.currentUnit(time="pal")
    elif frame_rate == 30:
        mc.currentUnit(time="ntsc")
    else:
        print "Warning, unsupported frame_rate", frame_rate

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
    ypr = []
    while kbb.read_next():
        if kbb.ltc.frames != skip_item:
            if frame%100==0:
                print kbb.header.msg_num, kbb.ltc, frame
            skip_item = kbb.ltc.frames
            mc.setKeyframe(kb_node, attribute="tc_h", time=frame, value=kbb.ltc.hours)
            mc.setKeyframe(kb_node, attribute="tc_m", time=frame, value=kbb.ltc.minutes)
            mc.setKeyframe(kb_node, attribute="tc_s", time=frame, value=kbb.ltc.seconds)
            mc.setKeyframe(kb_node, attribute="tc_f", time=frame, value=kbb.ltc.frames)
            for i in range(len(ypr)):
                sub_frame = float(i+1) / (float(len(ypr))) + (frame - 1)
                #print "\t", kbb.header.msg_num, kbb.ltc, frame, sub_frame
                if is_sane(ypr[i].yaw): mc.setKeyframe(kb_node, attribute="ry", time=sub_frame, value=-ypr[i].yaw)
                if is_sane(ypr[i].pitch): mc.setKeyframe(kb_node, attribute="rx", time=sub_frame, value=ypr[i].pitch)
                if is_sane(ypr[i].roll): mc.setKeyframe(kb_node, attribute="rz", time=sub_frame, value=180.0-ypr[i].roll)
            ypr = []
            frame += 1
        else:
            ypr.append(kbb.vnav)

    # get rid of the full-path using mc.ls()
    mc.filterCurve("%s_rotateY"%mc.ls(kb_node)[0])
    mc.filterCurve("%s_rotateX"%mc.ls(kb_node)[0])
    mc.filterCurve("%s_rotateZ"%mc.ls(kb_node)[0])
    mc.keyTangent("%s_rotateY"%mc.ls(kb_node)[0], outTangentType="linear")
    mc.keyTangent("%s_rotateX"%mc.ls(kb_node)[0], outTangentType="linear")
    mc.keyTangent("%s_rotateZ"%mc.ls(kb_node)[0], outTangentType="linear")

    mc.playbackOptions(minTime=1,maxTime=frame)


def mkScene():
    skydome = mc.polySphere(name="skydome", r=1000)
    shader = mc.shadingNode("blinn", asShader=True)
    checker = mc.shadingNode("file", asTexture=True)
    mc.setAttr("%s.fileTextureName"%checker,"c:/ChibiStudio/workspace/kuroBox/scripts/UV.png",type="string")
    mc.setAttr("%s.repeatUV"%checker, 10, 10)
    mc.connectAttr("%s.outColor"%checker, "%s.color"%shader, force=True)
    mc.select(skydome)
    mc.hyperShade(assign=shader)

"""
import sys
import maya.cmds as mc
if "c:/ChibiStudio/workspace/kuroBox/scripts/" not in sys.path:
    sys.path.append("c:/ChibiStudio/workspace/kuroBox/scripts/")
import maya_tools
maya_tools = reload(maya_tools)

mc.file( force=True, new=True )
maya_tools.mkScene()
name = "NB"
kb_node = maya_tools.mkCam(name,"c:/ChibiStudio/workspace/KUROBOX_TEST_DATA/2013-08-08/MVI_7892/out/MVI_7892.00001.jpg")
maya_tools.animateCam(kb_node, "c:/ChibiStudio/workspace/KUROBOX_TEST_DATA/2013-08-08/MVI_7892/KURO0046.KBB")
mc.setAttr("%s_imagePlaneShape.frameOffset"%name, -252)
"""