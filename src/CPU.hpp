#pragma once
#include <string>
#include <array>
#include <chrono>
#include <random>

#include "types.hpp"

constexpr u32 MEMORY_SIZE = 1024 * 4; // 4kB

class CPU
{
public:
    CPU()
        : gen((std::random_device{})()), distrib(0x0, 0xFF)
    {
        PC = 0x200;
        I = SP = delay_timer = sound_timer = 0;
        lastTime = std::chrono::steady_clock::now();

        mem.fill(0);
        V.fill(0);
        stack.fill(0);
        screen.fill(0);
        keys.fill(0);

        for(int i = 0; i < 80; i++)
          mem[0x050 + i] = fontset[i];
    }

      ~CPU() = default;

      bool loadRom(const std::string &filePath);

      void run();

      std::array<u8, MEMORY_SIZE> mem;
      std::array<u8, 16> V;
      std::array<u16, 16> stack;
      std::array<u8, (64*32)> screen;
      std::array<u8, 16> keys;


      u16 I; // Address register
      u16 PC;
      u8 SP;
      u8 delay_timer, sound_timer;
      int waiting_key{-1};


private:
    std::chrono::steady_clock::time_point lastTime;

    std::mt19937 gen;
    std::uniform_int_distribution<u16> distrib;

    constexpr static std::array<u8, 80> fontset = {
        0xF0, 0x90, 0x90, 0x90, 0xF0, // 0
        0x20, 0x60, 0x20, 0x20, 0x70, // 1
        0xF0, 0x10, 0xF0, 0x80, 0xF0, // 2
        0xF0, 0x10, 0xF0, 0x10, 0xF0, // 3
        0x90, 0x90, 0xF0, 0x10, 0x10, // 4
        0xF0, 0x80, 0xF0, 0x10, 0xF0, // 5
        0xF0, 0x80, 0xF0, 0x90, 0xF0, // 6
        0xF0, 0x10, 0x20, 0x40, 0x40, // 7
        0xF0, 0x90, 0xF0, 0x90, 0xF0, // 8
        0xF0, 0x90, 0xF0, 0x10, 0xF0, // 9
        0xF0, 0x90, 0xF0, 0x90, 0x90, // A
        0xE0, 0x90, 0xE0, 0x90, 0xE0, // B
        0xF0, 0x80, 0x80, 0x80, 0xF0, // C
        0xE0, 0x90, 0x90, 0x90, 0xE0, // D
        0xF0, 0x80, 0xF0, 0x80, 0xF0, // E
        0xF0, 0x80, 0xF0, 0x80, 0x80  // F
    };

};
