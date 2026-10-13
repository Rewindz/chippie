#include "CPU.hpp"

#include <chrono>
#include <fstream>
#include <iostream>


bool CPU::loadRom(const std::string &filePath)
{
    std::ifstream rom(filePath, std::ios::binary | std::ios::ate);
    if (rom.is_open()) {
        auto sz = rom.tellg();
        if (sz > (MEMORY_SIZE - 512)) {
          std::cerr << "ROM too large!" << std::endl;
          return false;
        }

        rom.seekg(0, std::ios::beg);
        if (rom.read(reinterpret_cast<char*>(mem.data() + 0x200), sz)) {
            return true;
        }
    }
    std::cerr << "Could not open ROM." << std::endl;
    return false;
}

void CPU::run()
{
    auto now = std::chrono::steady_clock::now();
    if (now - lastTime > std::chrono::microseconds(16666)) {
        if (sound_timer > 0)
            sound_timer--;
        if (delay_timer > 0)
            delay_timer--;
        lastTime = now;
    }
    u16 op = (mem[PC] << 8) | mem[PC + 1];
    PC += 2;

    u8 X = (op & 0x0F00) >> 8;
    u8 Y = (op & 0x00F0) >> 4;
    u8 NN = (op & 0x00FF);
    u16 NNN = (op & 0x0FFF);

    switch (op & 0xF000) {
        case 0x0000: {
            switch (NN) {
                case 0xEE:
                    if (SP > 0) {
                        PC = stack[--SP];
                    }
                    break;
                case 0xE0:
                    screen.fill(0);
                    break;
            }
            break;
        }
        case 0x2000:
            if (SP < stack.size()) {
                stack[SP++] = PC;
            }
            [[fallthrough]];
        case 0x1000:
            PC = NNN;
            break;
        case 0x3000: {
            if (V[X] == NN) {
                PC += 2;
            }
            break;
        }
        case 0x4000: {
            if (V[X] != NN) {
                PC += 2;
            }
            break;
        }
        case 0x5000: {
            if (V[X] == V[Y]) {
                PC += 2;
            }
            break;
        }
        case 0x6000: {
            V[X] = NN;
            break;
        }
        case 0x7000: {
            V[X] += NN;
            break;
        }
        case 0x8000: {
            switch (op & 0x000F) {
                case 0x0000: {
                    V[X] = V[Y];
                    break;
                }
                case 0x0001: {
                    V[X] |= V[Y];
                    break;
                }
                case 0x0002: {
                    V[X] &= V[Y];
                    break;
                }
                case 0x0003: {
                      V[X] ^= V[Y];
                      break;
                }
                case 0x0004: {
                      u16 sum = V[X] + V[Y];
                      u8 carry = sum > 255;
                      V[X] = sum & 0xFF;
                      V[0xF] = carry;
                      break;
                }
                case 0x0005: {
                      u8 carry = (V[X] >= V[Y]) ? 1 : 0;
                      V[X] -= V[Y];
                      V[0xF] = carry;
                      break;
                }
                case 0x0006: {
                      u8 carry = V[X] & 1;
                      V[X] >>= 1;
                      V[0xF] = carry;
                      break;
                }
                case 0x0007: {
                      u8 carry = (V[Y] >= V[X]) ? 1 : 0;
                      V[X] = V[Y] - V[X];
                      V[0xF] = carry;
                      break;
                }
                case 0x000E: {
                      u8 carry = (V[X] & 0b10000000) >> 7;
                      V[X] <<= 1;
                      V[0xF] = carry;
                      break;
                }
                default: {
                      std::cerr << "Not implemented 0x8xxx: " << std::hex << op << std::endl;
                      break;
                }
            }
            break;
        }
        case 0x9000: {
            if (V[X] != V[Y]) {
                PC += 2;
            }
            break;
        }
        case 0xA000: {
            I = NNN;
            break;
        }
        case 0xB000: {
            PC = V[0] + NNN;
            break;
        }
        case 0xC000: {
            V[X] = distrib(gen) & NN;
            break;
        }
        case 0xD000: {
            u8 N = op & 0x000F;

            u8 startX = V[X] % 64;
            u8 startY = V[Y] % 32;
            V[0xF] = 0;

            for (u8 row = 0; row < N; row++) {
                u8 sprite = mem[I + row];
                for (u8 col = 0; col < 8; col++) {
                    u8 pixel = sprite & (0b10000000 >> col);
                    if (pixel) {
                        u32 screenX = startX + col;
                        u32 screenY = startY + row;
                        if (screenX >= 64 || screenY >= 32) continue;
                        u32 screenIdx = screenY * 64 + screenX;
                        if (screen[screenIdx])
                              V[0xF] = 1;
                        screen[screenIdx] ^= 1;
                    }
                }
            }
            break;
        }
        case 0xE000: {
            switch (NN) {
                case 0x9E: {
                    if (keys[V[X]])
                        PC += 2;
                    break;
                }
                case 0xA1: {
                    if (!keys[V[X]])
                        PC += 2;
                    break;
                }
            }
            break;
        }
        case 0xF000: {
            switch (NN) {
                case 0x07: {
                    V[X] = delay_timer;
                    break;
                }
                case 0x0A: {
                    if (waiting_key == -1) {
                        for (int i = 0; i <= 0xF; i++) {
                            if (keys[i]) {
                                waiting_key = i;
                                break;
                            }
                        }
                        PC -= 2;
                    } else {
                        if (!keys[waiting_key]) {
                            V[X] = waiting_key;
                            waiting_key = -1;
                        } else {
                            PC -= 2;
                        }
                    }
                    break;
                }
                case 0x15: {
                    delay_timer = V[X];
                    break;
                }
                case 0x18: {
                    sound_timer = V[X];
                    break;
                }
                case 0x1E: {
                    I += V[X];
                    break;
                }
                case 0x29: {
                    u8 character = V[X] & 0x0F;
                    I = 0x050 + (character * 5);
                    break;
                }
                case 0x33: {
                    mem[I] = V[X] / 100;
                    mem[I + 1] = (V[X] % 100) / 10;
                    mem[I + 2] = V[X] % 10;
                    break;
                }
                case 0x55: {
                    for (u8 i = 0; i <= X; i++)
                        mem[I + i] = V[i];
                    break;
                }
                case 0x65: {
                    for (u8 i = 0; i <= X; i++)
                        V[i] = mem[I + i];
                    break;
                }
            }
            break;
        }
        default: {
            std::cerr << "TODO: " << std::hex << op << std::endl;
            break;
        }
    }
}
