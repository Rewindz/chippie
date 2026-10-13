# chipc - CHIP-8 Assembler Reference Manual

*chipc* is a assembler for CHIP-8 programs. It uses the 'Cowgod' CHIP-8 syntax.

## General Syntax

*   **Case Sensitivity:** Instruction mnemonics (`LD`, `ADD`, `I`) and registers (`V0`, `vF`) are case-insensitive. Text labels (`LOOP:`) are case-sensitive.
*   **Registers:** 16 general-purpose 8-bit registers (`V0` - `VF`) and one 16-bit index register (`I`).
*   **Literals:** Hexadecimal (`0xFF`, `0x200`) and Binary (`0b11110000`).
*   **Comments:** Anything following a semicolon `;` is ignored.

### Labels and Entry Point
The assembler injects a boot jump automatically. You **must** define a `_start:` label in your code, which acts as the entry point. Labels are defined with a trailing colon but referenced without it.

```nasm
_start:
    JP MAIN_LOOP

MAIN_LOOP:
    JP MAIN_LOOP
```

### Raw Data (`DB`)
Use `DB` (Define Byte) to insert raw data, such as sprites, directly into the ROM. It accepts hex or binary literals and can chain multiple bytes on a single line.

```nasm
PLAYER_SPRITE:
    DB 0b10000001
    DB 0b01000010
    DB 0xFF 0x3C 0x18  ; You can mix and match on the same line
```

---

## Instruction Set

### Program Flow

| Mnemonic | Opcode | Description |
|---|---|---|
| `CLS` | `00E0` | Clear the display. |
| `RET` | `00EE` | Return from a subroutine. |
| `JP addr` | `1nnn` | Jump to the specified address or label. |
| `CALL addr` | `2nnn` | Call subroutine at the specified address or label. |
| `JP V0, addr` | `Bnnn` | Jump to the specified address plus the value in `V0`. |

### Branching (Skips)

| Mnemonic | Opcode | Description |
|---|---|---|
| `SE Vx, byte` | `3xkk` | Skip the next instruction if `Vx` == `byte`. |
| `SNE Vx, byte` | `4xkk` | Skip the next instruction if `Vx` != `byte`. |
| `SE Vx, Vy` | `5xy0` | Skip the next instruction if `Vx` == `Vy`. |
| `SNE Vx, Vy` | `9xy0` | Skip the next instruction if `Vx` != `Vy`. |

### Register and Memory Loads

| Mnemonic | Opcode | Description |
|---|---|---|
| `LD Vx, byte` | `6xkk` | Set `Vx` to `byte`. |
| `LD Vx, Vy` | `8xy0` | Set `Vx` to the value of `Vy`. |
| `LD I, addr` | `Annn` | Set index register `I` to `addr` (hex or label). |
| `LD I, Vx` | `Fx55` | Store registers `V0` through `Vx` into memory starting at `I`. |
| `LD Vx, I` | `Fx65` | Read registers `V0` through `Vx` from memory starting at `I`. |
| `LD B, Vx` | `Fx33` | Store the Binary-Coded Decimal representation of `Vx` in memory at `I`, `I+1`, and `I+2`. |

### Math and Bitwise Operations

| Mnemonic | Opcode | Description |
|---|---|---|
| `ADD Vx, byte` | `7xkk` | Add `byte` to `Vx`. (Does *not* change the carry flag `VF`). |
| `ADD Vx, Vy` | `8xy4` | Add `Vy` to `Vx`. `VF` is set to 1 if there is a carry, 0 otherwise. |
| `ADD I, Vx` | `Fx1E` | Add `Vx` to the index register `I`. |
| `SUB Vx, Vy` | `8xy5` | Subtract `Vy` from `Vx`. `VF` is set to 0 if there is a borrow, 1 otherwise. |
| `SUBN Vx, Vy` | `8xy7` | Subtract `Vx` from `Vy` and store in `Vx`. `VF` is set to 0 if there is a borrow, 1 otherwise. |
| `OR Vx, Vy` | `8xy1` | Bitwise OR on `Vx` and `Vy`, stores result in `Vx`. |
| `AND Vx, Vy` | `8xy2` | Bitwise AND on `Vx` and `Vy`, stores result in `Vx`. |
| `XOR Vx, Vy` | `8xy3` | Bitwise XOR on `Vx` and `Vy`, stores result in `Vx`. |
| `SHR Vx {, Vy}` | `8xy6` | Shift `Vx` right by 1. `VF` is set to the least significant bit before the shift. *(Note: `Vy` is optional).* |
| `SHL Vx {, Vy}` | `8xyE` | Shift `Vx` left by 1. `VF` is set to the most significant bit before the shift. *(Note: `Vy` is optional).* |
| `RND Vx, byte` | `Cxkk` | Set `Vx` to a random byte (0-255) bitwise ANDed with `byte`. |

### Graphics and Input

| Mnemonic | Opcode | Description |
|---|---|---|
| `DRW Vx, Vy, n` | `Dxyn` | Draw an `n`-byte sprite at memory location `I` at screen coordinates (`Vx`, `Vy`). `VF` is set to 1 if any pixels are erased (collision). |
| `LD F, Vx` | `Fx29` | Set `I` to the memory address of the built-in hex font sprite for the character in `Vx`. |
| `SKP Vx` | `Ex9E` | Skip the next instruction if the key with the value of `Vx` is pressed. |
| `SKNP Vx` | `ExA1` | Skip the next instruction if the key with the value of `Vx` is NOT pressed. |
| `LD Vx, K` | `Fx0A` | Pause execution and wait for a key press, then store the key value in `Vx`. |

### Timers and Sound

| Mnemonic | Opcode | Description |
|---|---|---|
| `LD DT, Vx` | `Fx15` | Set the Delay Timer to the value in `Vx`. |
| `LD ST, Vx` | `Fx18` | Set the Sound Timer to the value in `Vx`. (Beeps as long as ST > 0). |
| `LD Vx, DT` | `Fx07` | Set `Vx` to the current value of the Delay Timer. |
