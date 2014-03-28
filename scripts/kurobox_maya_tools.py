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

"""
Usage: 

Make sure that the scripts/ directory is in your PYTHONPATH. You can do this 
manually by running this command in your maya python script window:

##-------------------------------------------
import sys
sys.path.append("<PATH_TO_SCRIPTS>")
##-------------------------------------------

Then execute the following in maya python script window:

##-------------------------------------------
import kurobox_maya_tools as kb

kb.setRoot("<PATH_TO_DATA>")
kb.mkScene("<PATH_TO_SCRIPTS>/UV.png")
kb_node = kb.mkCam("camera_name", "<FIRST_FRAME_OF_IMAGE_SEQUENCE>")
kb.animateCam(kb_node, "<KBB_FILENAME>", frame_rate=30, start_from="00:00:00:00", total_frames=-1)
##-------------------------------------------

The "frame_rate" parameter is optional, but uses 30(NTSC) as default. The start_from & total_frames are
also optional, and if left out will just import and create a track for the entire KBB file - which may
be hours long, so beware!

Also, if you only want to see the motion without loading any frames, the <FIRST_FRAME_OF_IMAGE_SEQUENCE>
parameter is also optional.

Here is a complete example to paste in the python script window:

##-------------------------------------------
import sys
sys.path.append("c:/data/kurobox/Maya Scripts/")
import kurobox_maya_tools as kb
kb.setRoot("c:/data/kurobox/data")
kb.mkScene("c:/data/kurobox/UV.png")
kb_node = kb.mkCam("engine_test", "2014-03-28/SC0001_SH0001_TK0001.00000.JPG")
kb.animateCam(kb_node,"KURO0002.KBB", frame_rate=30, start_from="00:00:00:00", total_frames=-1)
##-------------------------------------------

"""

import sys
import os

import maya.cmds as mc
import KBB
import KBB_util
import KBB_types

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

    if total_frames == -1:
        total_frames = None

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
        return
    print kbb
    ##############kbb.set_index(0)
    first_tc_idx = 0
    while kbb.read_next():
        if kbb.packet.header.msg_class == KBB_types.CLASS_DATA:
            if kbb.packet.ltc_frame.hours != 0 or kbb.packet.ltc_frame.minutes != 0 or kbb.packet.ltc_frame.seconds != 0 or kbb.packet.ltc_frame.frames != 0:
                break
        first_tc_idx += 1
    print "First TC index:", first_tc_idx, kbb.packet.ltc_frame
    skip_item = None
    ypr = []
    skipping = True
    start_frame = kbb.packet.ltc_frame.frame_number(frame_rate)
    frame_count = 0
    first_frame = None
    while kbb.read_next():
        if KBB_util.LTC_LT(kbb.packet.ltc_frame, start_ltc):
            continue

        if first_frame is None:
            first_frame = kbb.packet.ltc_frame.frame_number(frame_rate)
        skipping = False
        #print kbb.header.msg_num, kbb.ltc_frame, frame, frames
        if kbb.packet.ltc_frame.frames != skip_item:
            if frame_count%100==0:
                print kbb.packet.msg_num, kbb.packet.ltc_frame, frame_count
            skip_item = kbb.packet.ltc_frame.frames
            frame = kbb.packet.ltc_frame.frame_number(frame_rate) - first_frame + 1
            mc.setKeyframe(kb_node, attribute="tc_h", time=frame, value=kbb.packet.ltc_frame.hours)
            mc.setKeyframe(kb_node, attribute="tc_m", time=frame, value=kbb.packet.ltc_frame.minutes)
            mc.setKeyframe(kb_node, attribute="tc_s", time=frame, value=kbb.packet.ltc_frame.seconds)
            mc.setKeyframe(kb_node, attribute="tc_f", time=frame, value=kbb.packet.ltc_frame.frames)
            mc.setKeyframe(kb_node, attribute="msg_num", time=frame, value=kbb.packet.msg_num)
            for i in range(len(ypr)):
                sub_frame = float(i+1) / (float(len(ypr))) + (frame - 1)
                #print "\t", kbb.header.msg_num, kbb.ltc_frame, frame, sub_frame
                if is_sane(ypr[i].yaw): mc.setKeyframe(kb_node, attribute="ry", time=sub_frame, value=-ypr[i].yaw)
                if is_sane(ypr[i].pitch): mc.setKeyframe(kb_node, attribute="rx", time=sub_frame, value=ypr[i].pitch)
                if is_sane(ypr[i].roll): mc.setKeyframe(kb_node, attribute="rz", time=sub_frame, value=180.0-ypr[i].roll)
            ypr = []
            frame_count += 1
            if total_frames is not None:
                if frame_count > total_frames:
                    break
        else:
            ypr.append(kbb.packet.vnav)
    end_frame = kbb.packet.ltc_frame.frame_number(frame_rate)

    # get rid of the full-path using mc.ls()
    mc.filterCurve("%s_rotateY"%mc.ls(kb_node)[0])
    mc.filterCurve("%s_rotateX"%mc.ls(kb_node)[0])
    mc.filterCurve("%s_rotateZ"%mc.ls(kb_node)[0])
    mc.keyTangent("%s_rotateY"%mc.ls(kb_node)[0], outTangentType="linear")
    mc.keyTangent("%s_rotateX"%mc.ls(kb_node)[0], outTangentType="linear")
    mc.keyTangent("%s_rotateZ"%mc.ls(kb_node)[0], outTangentType="linear")

    mc.playbackOptions(minTime=0,maxTime=(end_frame-first_frame+1))


def mkScene(teximg):
    if ROOT_DIR is not None:
        teximg = os.path.join(ROOT_DIR,teximg)
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
