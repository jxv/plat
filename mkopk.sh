rm -r dist plat.opk
# make clean_gcw0
make gcw0
mkdir dist
mkdir dist/res
cp plat dist
cp icon.png dist
cp default.gcw0.desktop dist
cp -r res/*.bmp dist/res
cp -r res/*.png dist/res
cp -r res/*.xm dist/res
cp -r res/*.wav dist/res
cp -r res/*.txt dist/res
cp -r res/shader dist/res/shader
mksquashfs dist plat.opk -all-root -noappend -no-exports -no-xattrs
