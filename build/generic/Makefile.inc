
LIB=libcore.a
LIBSO=libxvidcore.so


SRCDIR = ../../src

SRC =	$(SRCDIR)/common/fdct.c \
	$(SRCDIR)/common/idct.c \
	$(SRCDIR)/common/interpolate8x8.c \
	$(SRCDIR)/common/quantize.c \
	$(SRCDIR)/common/quantize4.c \
	$(SRCDIR)/common/transfer.c \
	$(SRCDIR)/common/common.c \
	$(SRCDIR)/common/timer.c \
	$(SRCDIR)/decoder/bitreader.c \
	$(SRCDIR)/decoder/decoder.c \
	$(SRCDIR)/decoder/mbdecoding.c \
	$(SRCDIR)/decoder/mbdeprediction.c \
	$(SRCDIR)/encoder/cbp.c \
	$(SRCDIR)/encoder/sad.c \
	$(SRCDIR)/encoder/bitwriter.c \
	$(SRCDIR)/encoder/encoder.c \
	$(SRCDIR)/encoder/mbcoding.c \
	$(SRCDIR)/encoder/mbmotionestcomp.c \
	$(SRCDIR)/encoder/mbprediction.c \
	$(SRCDIR)/encoder/mbtransquant.c \
	$(SRCDIR)/encoder/putvlc.c \
	$(SRCDIR)/encoder/ratecontrol.c \
	$(SRCDIR)/image/halfpel.c \
	$(SRCDIR)/image/in.c \
	$(SRCDIR)/image/out.c \
	$(SRCDIR)/image/halfpel.c \
	$(SRCDIR)/image/image.c \
	$(SRCDIR)/image/in.c \
	$(SRCDIR)/image/out.c \
	$(SRCDIR)/opendivx.c \
	$(SRCDIR)/xvid.c

DIRS = $(SRCDIR)/*.o \
	$(SRCDIR)/common/*.o \
	$(SRCDIR)/decoder/*.o \
	$(SRCDIR)/encoder/*.o \
	$(SRCDIR)/image/*.o

SRC_INTEL = \
	$(SRCDIR)/common/intel_mmx/cpuid.asm \
	$(SRCDIR)/common/intel_mmx/fdct_mmx.asm \
	$(SRCDIR)/common/intel_mmx/idct_mmx.asm \
	$(SRCDIR)/common/intel_mmx/interpolate8x8_mmx.asm \
	$(SRCDIR)/common/intel_mmx/quantize_mmx.asm \
	$(SRCDIR)/common/intel_mmx/quantize4_mmx.asm \
	$(SRCDIR)/common/intel_mmx/transfer_mmx.asm \
	$(SRCDIR)/encoder/intel_mmx/cbp_mmx.asm \
	$(SRCDIR)/encoder/intel_mmx/sad_mmx.asm \
	$(SRCDIR)/image/intel_mmx/halfpel_mmx.asm \
	$(SRCDIR)/image/intel_mmx/in_rgb_mmx.asm \
	$(SRCDIR)/image/intel_mmx/in_yuv_mmx.asm \
	$(SRCDIR)/image/intel_mmx/in_yuyv_mmx.asm \
	$(SRCDIR)/image/intel_mmx/out_rgb24_mmx.asm \
	$(SRCDIR)/image/intel_mmx/out_rgb32_mmx.asm \
	$(SRCDIR)/image/intel_mmx/out_yuyv_mmx.asm

DIRS_INTEL = \
	$(SRCDIR)/common/intel_mmx/*.o \
	$(SRCDIR)/decoder/intel_mmx/*.o \
	$(SRCDIR)/encoder/intel_mmx/*.o \
	$(SRCDIR)/image/intel_mmx/*.o

