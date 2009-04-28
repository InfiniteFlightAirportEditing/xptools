#!/bin/sh

# usage: bundle.sh product platform version

case "$2" in
"")
	echo "Second parameter must be mac|win|lin"
	exit 1
	;;
mac)
	build_dir=../build/Release
	;;
win)
	build_dir=../build/mingw/release_opt
	;;
lin)
	build_dir=../build/linux/release_opt
	;;
esac

case "$1" in
"")
	echo "First parameter must be ac3d|meshtool"
	exit 1
	;;
ac3d)
	out_file="ac3d_plugin_"$2"_"$3".zip"
	src_files="$build_dir/XPlaneSupportMac.p ../src/AC3DPlugins/README ../src/AC3DPlugins/XPlaneSupport.tcl"
	;;
meshtool)
	out_file="meshtool_"$2"_"$3".zip"
	src_files="$build_dir/MeshTool ../src/MeshTool/README.meshtool"
	;;
esac

rm $out_file
zip -j $out_file $src_files
scp $out_file bsupnik@dev.x-plane.com:/shared/download/tools/