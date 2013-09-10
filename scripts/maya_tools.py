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
import os
if "c:/ChibiStudio/workspace/kuroBox/scripts/" not in sys.path:
    sys.path.append("c:/ChibiStudio/workspace/kuroBox/scripts/")

import maya.cmds as mc
import KBB
import KBB_util

ROOT_DIR = None

def setRoot(root):
    global ROOT_DIR
    ROOT_DIR = root

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
        if ROOT_DIR is not None:
            img_plane_imgs = os.path.join(ROOT_DIR,img_plane_imgs)
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
    mc.addAttr(camera_node, longName="msg_num", attributeType="long")

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

def animateCam(kb_node, kbb_file, frame_rate=30, start_from=None, total_frames=None):
    if frame_rate == 24:
        mc.currentUnit(time="film")
    elif frame_rate == 25:
        mc.currentUnit(time="pal")
    elif frame_rate == 30:
        mc.currentUnit(time="ntsc")
    else:
        print "Warning, unsupported frame_rate", frame_rate

    start_ltc = KBB_util.LTC()
    if start_from is not None:
        start_ltc = KBB_util.LTC(start_from)

    print "STARTING FROM:", start_ltc, start_from

    if ROOT_DIR is not None:
        kbb_file = os.path.join(ROOT_DIR,kbb_file)
    try:
        kbb = KBB.KBB_factory(kbb_file)
    except IOError:
        print "ERROR, couldn't open file '%s'"%kbb_file
    print kbb
    kbb.set_index(0)
    first_tc_idx = 0
    while kbb.read_next():
        if kbb.ltc.hours != 0 or kbb.ltc.minutes != 0 or kbb.ltc.seconds != 0 or kbb.ltc.frames != 0:
            break
        first_tc_idx += 1
    print "First TC index:", first_tc_idx, kbb.ltc
    skip_item = None
    ypr = []
    skipping = True
    start_frame = kbb.ltc.frame_number(frame_rate)
    frame_count = 0
    while kbb.read_next():
        if KBB_util.LTC_LT(kbb.ltc, start_ltc):
            continue
        skipping = False
        #print kbb.header.msg_num, kbb.ltc, frame, frames
        if kbb.ltc.frames != skip_item:
            if frame_count%100==0:
                print kbb.header.msg_num, kbb.ltc, frame_count
            skip_item = kbb.ltc.frames
            frame = kbb.ltc.frame_number(frame_rate)
            mc.setKeyframe(kb_node, attribute="tc_h", time=frame, value=kbb.ltc.hours)
            mc.setKeyframe(kb_node, attribute="tc_m", time=frame, value=kbb.ltc.minutes)
            mc.setKeyframe(kb_node, attribute="tc_s", time=frame, value=kbb.ltc.seconds)
            mc.setKeyframe(kb_node, attribute="tc_f", time=frame, value=kbb.ltc.frames)
            mc.setKeyframe(kb_node, attribute="msg_num", time=frame, value=kbb.header.msg_num)
            for i in range(len(ypr)):
                sub_frame = float(i+1) / (float(len(ypr))) + (frame - 1)
                #print "\t", kbb.header.msg_num, kbb.ltc, frame, sub_frame
                if is_sane(ypr[i].yaw): mc.setKeyframe(kb_node, attribute="ry", time=sub_frame, value=-ypr[i].yaw)
                if is_sane(ypr[i].pitch): mc.setKeyframe(kb_node, attribute="rx", time=sub_frame, value=ypr[i].pitch)
                if is_sane(ypr[i].roll): mc.setKeyframe(kb_node, attribute="rz", time=sub_frame, value=180.0-ypr[i].roll)
            ypr = []
            frame_count += 1
            if total_frames is not None:
                if frame_count > total_frames:
                    break
        else:
            ypr.append(kbb.vnav)
    end_frame = kbb.ltc.frame_number(frame_rate)

    # get rid of the full-path using mc.ls()
    mc.filterCurve("%s_rotateY"%mc.ls(kb_node)[0])
    mc.filterCurve("%s_rotateX"%mc.ls(kb_node)[0])
    mc.filterCurve("%s_rotateZ"%mc.ls(kb_node)[0])
    mc.keyTangent("%s_rotateY"%mc.ls(kb_node)[0], outTangentType="linear")
    mc.keyTangent("%s_rotateX"%mc.ls(kb_node)[0], outTangentType="linear")
    mc.keyTangent("%s_rotateZ"%mc.ls(kb_node)[0], outTangentType="linear")

    mc.playbackOptions(minTime=0,maxTime=end_frame)
#    mc.playbackOptions(minTime=start_frame,maxTime=end_frame)


def mkScene(teximg="c:/ChibiStudio/workspace/kuroBox/scripts/UV.png"):
    skydome = mc.polySphere(name="skydome", r=1000)
    shader = mc.shadingNode("blinn", asShader=True)
    checker = mc.shadingNode("file", asTexture=True)
    mc.setAttr("%s.fileTextureName"%checker, teximg, type="string")
    mc.setAttr("%s.repeatUV"%checker, 10, 10)
    mc.connectAttr("%s.outColor"%checker, "%s.color"%shader, force=True)
    mc.select(skydome)
    mc.hyperShade(assign=shader)

def mkK(name,img_plane_imgs,kbb):
    kb_node = mkCam(name,img_plane_imgs)
    animateCam(kb_node,kbb)
    mc.setAttr("%s_imagePlaneShape.frameOffset"%name, -252)
    return kb_node

def dupK(oldname,newname,img_plane_imgs):
    old_part = oldname[:-4] # get rid of the "_cam"
    print old_part
    newnode = mc.duplicate(oldname, name="%s_cam"%newname, upstreamNodes=True, returnRootsOnly=True)
    print "New node:", newnode
    for i in mc.listRelatives(newnode, shapes=False, fullPath=True): 
        print i, i.replace("|%s|"%newnode[0],"").replace(old_part,newname)
        mc.rename(i,i.replace("|%s|"%newnode[0],"").replace(old_part,newname))
    imgPlane = mc.ls("%s*imagePlane"%newname)[0]
    mc.setAttr( "%s.imageName" % imgPlane, img_plane_imgs, type="string")
    return newnode


"""
import sys
import maya.cmds as mc
if "c:/ChibiStudio/workspace/kuroBox/scripts/" not in sys.path:
    sys.path.append("c:/ChibiStudio/workspace/kuroBox/scripts/")

import maya_tools
maya_tools = reload(maya_tools)

mc.file( force=True, new=True )
maya_tools.mkScene()
kb_node = maya_tools.mkK("DSC_0018","c:/ChibiStudio/workspace/KUROBOX_TEST_DATA/2013-08-21/DSC_0018/DSC_0018.00001.jpg",\
    "c:/ChibiStudio/workspace/KUROBOX_TEST_DATA/2013-08-21/KURO0019.KBB")
for i in ["DSC_0020","DSC_0022","DSC_0025","DSC_0026","DSC_0028","DSC_0029","DSC_0030","DSCN7309","DSCN7310","DSCN7311","DSCN7312","DSCN7313"]:
    maya_tools.dupK(kb_node, i,"c:/ChibiStudio/workspace/KUROBOX_TEST_DATA/2013-08-21/%s/%s.00001.jpg"%(i,i)


-------------------------------------------------


mc.setAttr("%s_imagePlaneShape.frameOffset"%name, -252)


-------------------------------------------------

import sys
import maya.cmds as mc
if "c:/ChibiStudio/workspace/kuroBox/scripts/" not in sys.path:
    sys.path.append("c:/ChibiStudio/workspace/kuroBox/scripts/")

import maya_tools
maya_tools = reload(maya_tools)

mc.file( force=True, new=True )
maya_tools.mkScene()

kb_node = maya_tools.mkCam("DSC_0018","c:/ChibiStudio/workspace/KUROBOX_TEST_DATA/2013-08-21/DSC_0018/DSC_0018.00001.jpg")
maya_tools.animateCam(kb_node,"c:/ChibiStudio/workspace/KUROBOX_TEST_DATA/2013-08-21/KURO0019.KBB",start_from="00:31:00:00")

"""