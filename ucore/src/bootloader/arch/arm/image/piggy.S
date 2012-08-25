.section .piggydata,#alloc
.globl input_data
input_data:
.incbin "bin/kernel"
.globl input_datainput_data_end
input_data_end:
