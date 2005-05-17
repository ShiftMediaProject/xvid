#
# List of benches to run
#

#########################################
# Decoder benches
#########################################

  # Raw command-line args passed to 'xvid_bench 9'
  # format: bitstream_name width height checksum
  # followed, possibly, by the CPU option to use.  

@Dec_Benches = (
  "lowdelay.m4v 720 576 0xf2a3229d -sse2"
, "lowdelay.m4v 720 576 0x5ea8e958 -c"
, "test_lowdelay.m4v 720 576 0x5ea8e958 -mmx"
);

#########################################
