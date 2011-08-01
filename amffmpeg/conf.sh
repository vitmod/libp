#!/bin/bash


PREBUILT=$NDK_ROOT/build/prebuilt/linux-x86/arm-eabi-4.4.0
PLATFORM=$NDK_ROOT/build/platforms/android-8/arch-arm

./configure \
	--cc=$PREBUILT/bin/arm-eabi-gcc \
	--arch=arm \
	--enable-cross-compile --cross-prefix=$PREBUILT/bin/arm-eabi- \
	--prefix=$PREFIX \
	--incdir=$PLATFORM/usr/include \
	--extra-ldflags="-Wl,-T,$PREBUILT/arm-eabi/lib/ldscripts/armelf.x -Wl,-rpath-link=$PLATFORM/usr/lib -L$PLATFORM/usr/lib -nostdlib $PREBUILT/lib/gcc/arm-eabi/4.4.0/crtbegin.o $PREBUILT/lib/gcc/arm-eabi/4.4.0/crtend.o -lc -lm -ldl" \
	--disable-static --enable-shared \
	--disable-ffplay --disable-ffserver \
	--disable-doc \
	--disable-mpegaudio-hp \
	--disable-encoders \
	--disable-decoder=h264 \
	--disable-muxers \
	--disable-filters \
	--disable-altivec \
	--disable-amd3dnow \
	--disable-amd3dnowext \
	--disable-mmx --disable-mmx2 --disable-sse --disable-ssse3 \
	--disable-armv5te --disable-armv6 --disable-armv6t2 --disable-armvfp \
	--disable-iwmmxt --disable-mmi --disable-vis --disable-yasm \
	--enable-pic

find . -name Makefile -print0 | xargs -0 sed -i '/..\/subdir.mak/d'
find . -name Makefile -print0 | xargs -0 sed -i '/..\/config.mak/d'
sed -i 's/restrict restrict/restrict/' config.h

