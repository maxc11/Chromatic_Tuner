# 
# Usage: To re-create this platform project launch xsct with below options.
# xsct D:\vitisworkspace2lab2b\lab2bplat\platform.tcl
# 
# OR launch xsct and run below command.
# source D:\vitisworkspace2lab2b\lab2bplat\platform.tcl
# 
# To create the platform in a different location, modify the -out option of "platform create" command.
# -out option specifies the output directory of the platform project.

platform create -name {lab2bplat}\
-hw {D:\project_1\system_wrapper.xsa}\
-proc {microblaze_0} -os {standalone} -out {D:/vitisworkspace2lab2b}

platform write
platform generate -domains 
platform active {lab2bplat}
bsp reload
bsp setdriver -ip sevenSeg_0 -driver generic -ver 3.0 3.1
bsp write
bsp reload
catch {bsp regenerate}
bsp reload
platform generate
platform generate
platform generate -domains standalone_domain 
platform active {lab2bplat}
platform config -updatehw {C:/Users/jgianelli/Downloads/project_1.xpr-lab3a/project_1.xpr/project_1/system_wrapper.xsa}
bsp reload
bsp setdriver -ip sevenSeg_0 -driver none -ver {}
bsp write
bsp reload
catch {bsp regenerate}
platform generate
platform generate
platform generate
platform generate
platform generate
platform generate -domains standalone_domain 
platform generate
platform generate
platform generate
platform generate
platform generate
