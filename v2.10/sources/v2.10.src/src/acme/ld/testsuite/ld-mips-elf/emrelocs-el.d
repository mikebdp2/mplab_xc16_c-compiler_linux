#name: MIPS 32-bit ELF embedded relocs (little endian)
#source: emrelocs1.s -EL
#source: emrelocs2.s -EL
#ld: -EL --embedded-relocs -T $srcdir/$subdir/emrelocs.ld
#objdump: -s

.*:     file format elf.*mips.*

Contents of section \.text:
 100000 00000000 01000000 00000000 00000000  .*
 100010 00000000 00000000 03000000 00000000  .*
Contents of section \.data:
 200000 00000000 02000000 00000000 00000000  .*
 200010 00000000 00000000 04000000 00000000  .*
 200020 04001000 04002000 18001000 18002000  .*
 200030 04001000 00000000 04002000 00000000  .*
 200040 18001000 00000000 18002000 00000000  .*
Contents of section \.rel\.sdata:
 300000 20000000 2e746578 74000000 24000000  .*
 300010 2e646174 61000000 28000000 2e746578  .*
 300020 74000000 2c000000 2e646174 61000000  .*
 300030 31000000 2e746578 74000000 39000000  .*
 300040 2e646174 61000000 41000000 2e746578  .*
 300050 74000000 49000000 2e646174 61000000  .*
#pass