SRCDIR = ../../src

SRC_GENERIC = \
 	$(SRCDIR)/decoder.c \
	$(SRCDIR)/encoder.c \
	$(SRCDIR)/xvid.c \
	$(SRCDIR)/bitstream/bitstream.c \
	$(SRCDIR)/bitstream/cbp.c \
	$(SRCDIR)/bitstream/mbcoding.c \
	$(SRCDIR)/dct/fdct.c \
	$(SRCDIR)/dct/idct.c \
	$(SRCDIR)/image/colorspace.c \
	$(SRCDIR)/image/image.c \
	$(SRCDIR)/image/interpolate8x8.c \
	$(SRCDIR)/motion/motion_comp.c \
	$(SRCDIR)/motion/motion_est.c \
	$(SRCDIR)/motion/sad.c \
	$(SRCDIR)/prediction/mbprediction.c \
	$(SRCDIR)/quant/adapt_quant.c \
	$(SRCDIR)/quant/quant_h263.c \
	$(SRCDIR)/quant/quant_matrix.c \
	$(SRCDIR)/quant/quant_mpeg4.c \
	$(SRCDIR)/utils/emms.c \
	$(SRCDIR)/utils/mbtransquant.c \
	$(SRCDIR)/utils/mem_align.c \
	$(SRCDIR)/utils/mem_transfer.c \
	$(SRCDIR)/utils/ratecontrol.c \
	$(SRCDIR)/utils/timer.c

SRC_DIVX4COMPAT = \
	$(SRCDIR)/divx4.c

SRC_IA32 = \
	$(SRCDIR)/utils/x86_asm/cpuid.asm \
	\
	$(SRCDIR)/bitstream/x86_asm/cbp_mmx.asm \
	$(SRCDIR)/bitstream/x86_asm/cbp_sse2.asm \
	$(SRCDIR)/dct/x86_asm/fdct_mmx.asm \
	$(SRCDIR)/dct/x86_asm/idct_mmx.asm \
	$(SRCDIR)/image/x86_asm/interpolate8x8_mmx.asm \
	$(SRCDIR)/image/x86_asm/rgb_to_yv12_mmx.asm \
	$(SRCDIR)/image/x86_asm/yuv_to_yv12_mmx.asm \
	$(SRCDIR)/image/x86_asm/yuyv_to_yv12_mmx.asm \
	$(SRCDIR)/image/x86_asm/yv12_to_rgb24_mmx.asm \
	$(SRCDIR)/image/x86_asm/yv12_to_rgb32_mmx.asm \
	$(SRCDIR)/image/x86_asm/yv12_to_yuyv_mmx.asm \
	$(SRCDIR)/motion/x86_asm/sad_mmx.asm \
	$(SRCDIR)/quant/x86_asm/quantize4_mmx.asm \
	$(SRCDIR)/quant/x86_asm/quantize_mmx.asm \
	$(SRCDIR)/utils/x86_asm/mem_transfer_mmx.asm	\
	\
	$(SRCDIR)/image/x86_asm/interpolate8x8_xmm.asm	\
	$(SRCDIR)/motion/x86_asm/sad_xmm.asm \
	$(SRCDIR)/motion/x86_asm/sad_sse2.asm \
	\
	$(SRCDIR)/image/x86_asm/interpolate8x8_3dn.asm \
	$(SRCDIR)/motion/x86_asm/sad_3dn.asm

SRC_IA64 = \
	$(SRCDIR)/dct/ia64_asm/fdct_ia64.s \
	$(SRCDIR)/image/ia64_asm/interpolate8x8_ia64.s \
	$(SRCDIR)/motion/ia64_asm/sad_ia64.s \
	$(SRCDIR)/motion/ia64_asm/halfpel8_refine_ia64.s \
	$(SRCDIR)/quant/ia64_asm/quant_h263_ia64.s \
	$(SRCDIR)/utils/ia64_asm/mem_transfer_ia64.s \

SRC_PPC = \
	$(SRCDIR)/bitstream/ppc_asm/cbp_ppc.s

SRC_ALTIVEC = \
	$(SRCDIR)/bitstream/ppc_asm/cbp_altivec.s \
	$(SRCDIR)/dct/ppc_asm/fdct_altivec.s \
	$(SRCDIR)/dct/ppc_asm/idct_altivec.s \
	$(SRCDIR)/motion/ppc_asm/sad_altivec.s
