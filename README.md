# d610-patch

Makes it possible to set unsupported vidio resolutions with intel chipset.

Old, source code no longer exist anywhere else online (to my knowledge). 

```
/*  resolution patch for 845G/855GM/865G
*
*       Fixes the i810-series video BIOS to support other resolution
*       like 1280x768, 1024x600. Currently only tested against the
*       845G VBIOS, revision #2686 #2720 and the 865G VBIOS, revision
*       #2831 #2919 #3144.
*
*       *VERY* likely that this won't work yet on any other revisions
*       or chipsets!!!
*
*       This code is based on the technique used in 845Gpatch and 1280
*       patch for 855GM. Many thanks to Christian Zietz <czietz@gmx.net>
*       and Andrew Tipton <andrewtipton@null.li>.
*
*       resolution patch for 845G/855GM/865G is public domain source code.
*       Use at your own risk!
*
*       Leslie Chu <kongkong_chu@yahoo.ca>
*
*       Add NetBSD/FreeBSD support by FUKAUMI Naoki <fun AT naobsd DOT org>.
*
*       Original code:
*         http://kongkong.no-ip.info:8080/kongkong/1280patch-845g-855gm-865g.c
*       Discussion:
*         http://www.leog.net/fujp_forum/topic.asp?TOPIC_ID=5371
*
*/
```
