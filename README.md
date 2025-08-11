<p align="center">
  <img src="docs/logo.svg" width="300"/><br/><br/>
  Tiny symmetric encryption that may run everywhere.<br/><br/>
</p>

[![Tests](https://github.com/Artem-Shapovalov/encodex/actions/workflows/testing.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/testing.yml)
[![ANSI Compliance](https://github.com/Artem-Shapovalov/encodex/actions/workflows/ansi_compliance.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/ansi_compliance.yml)
[![MISRA C 2012 Compliance](https://github.com/Artem-Shapovalov/encodex/actions/workflows/misra_compliance.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/misra_compliance.yml)

| Compiler          | Build | Text | Data | BSS  |
|-------------------|-------|------|------|------|
| gcc               | [![GCC Build](https://github.com/Artem-Shapovalov/encodex/actions/workflows/gcc_build.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/gcc_build.yml) | 1796 |    4 |    0 |
| clang             | [![Clang build](https://github.com/Artem-Shapovalov/encodex/actions/workflows/clang_build.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/clang_build.yml) | 1410 |    0 |    0 |
| arm-none-eabi-gcc | [![ARM Build](https://github.com/Artem-Shapovalov/encodex/actions/workflows/arm_build.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/arm_build.yml) | 1168 |    4 |    0 |
| avr-gcc           | [![AVR build](https://github.com/Artem-Shapovalov/encodex/actions/workflows/avr_build.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/avr_build.yml) | 1946 |    4 |    0 |
| riscv64-linux-gnu | [![RISC V Build](https://github.com/Artem-Shapovalov/encodex/actions/workflows/risc_v_build.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/risc_v_build.yml) | 2044 |    4 |    0 |
| mips-linux-gnu    | [![MIPS Build](https://github.com/Artem-Shapovalov/encodex/actions/workflows/Mips_check.yml/badge.svg)](https://github.com/Artem-Shapovalov/encodex/actions/workflows/Mips_check.yml) | 1936 |    4 |    0 |

# DISCLAIMER
All locks protect only from honest people. Besides this cypher is good enough it may be broken. It's not certified, use it at your own risk. I doing all to decrease your risks.

# Algorithm

ENCODEX is a lightweight and fast block cipher symmetrical key algorithm. The goal is to effectively cipher text in a single round, this why it may be effectively used on a low-performance devices. It operates 256 bytes blocks and keys. The source of entropy is a simple shifting pseudo-random generator.

The algorithm consists of next operations:

- RoL block: cyclic rotate bytes according the key.
- Add key: adding with overflow bytes of the key to the block.
- Noize: XOR with pseudo-random sequence all of the bytes.
- Shuffle: permutate bytes in the block in chaotic order.

| Stage         | Image 1                               | Image 2                             |
|---------------|---------------------------------------|-------------------------------------|
| Initial state | ![image](docs/portrait.png)           | ![image](docs/teapot.png)           |
| RoL block     | ![image](docs/portrait_rol_block.png) | ![image](docs/teapot_rol_block.png) |
| Add key       | ![image](docs/portrait_add_key.png)   | ![image](docs/teapot_add_key.png)   |
| Noize         | ![image](docs/portrait_noize.png)     | ![image](docs/teapot_noize.png)     |
| Shuffle       | ![image](docs/portrait_shuffle.png)   | ![image](docs/teapot_shuffle.png)   |

CBC mode is simple, it's such a pseudo-random key regeneration, this why you easily may encode and decode series of blocks in forward direction.

# Comparsion

Visual demonstration of each algorithm is much more comprehensive than just a raw data dumps. All three algorithms are conjuncted with permutations and each have some advantages and disadvantages with different kinds of the images.

ENCODEX shuffles image not so good as DES and AES because it works with a single round, but it works better with the monochrome images, because it distorts the data and equalizes probability of appearing of distorted data. This why, the portrait almost disappears in the repeating patter. Also, besides the pattern is repeating in all of three cases, you may notice that difference of color of encoded black and white for AES and DES, but ENCODEX have no visual difference between encoded black and white.

ENCODEX weaker in grayscale images because of visible patterns, but it still good enough, you can't identify teapot or cup. It's a product of a single round limitation.

CBC mode is good for all three ciphers and looks like a white noize.

| Mode | DES                                 | Encodex                                 | AES                                 |
|------|-------------------------------------|-----------------------------------------|-------------------------------------|
| ECB  | ![image](docs/portrait_des.png)     | ![image](docs/portrait_encoded.png)     | ![image](docs/portrait_aes.png)     |
| CBC  | ![image](docs/portrait_des_cbc.png) | ![image](docs/portrait_encoded_cbc.png) | ![image](docs/portrait_aes_cbc.png) |
| ECB  | ![image](docs/teapot_des.png)       | ![image](docs/teapot_encoded.png)       | ![image](docs/teapot_aes.png)       |
| CBC  | ![image](docs/teapot_des_cbc.png)   | ![image](docs/teapot_encoded_cbc.png)   | ![image](docs/teapot_aes_cbc.png)   |

# Usage

This algorithm is not certified at all, but it checked statically with MISRA C 2012 rules. It does not have any dependencies except C standard library. It needed for standard integer types. This code is written with ISO/ANSI C maneer and tested for compliance. This way you may use it in any project with any hardware.

To embed it in your project, just copy encodex.h and encodex.c and add it to your build system. Follow the doxygen comments in header file. Take a look on example application and tests.
