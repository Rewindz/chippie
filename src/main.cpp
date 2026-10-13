#include "CPU.hpp"
#include <raylib.h>

#include <array>

struct Keybinding
{
    int code, binding;
};

constexpr Keybinding key_bindings[] =
{
      {0x0, KEY_X},
      {0x1, KEY_ONE},
      {0x2, KEY_TWO},
      {0x3, KEY_THREE},
      {0x4, KEY_Q},
      {0x5, KEY_W},
      {0x6, KEY_E},
      {0x7, KEY_A},
      {0x8, KEY_S},
      {0x9, KEY_D},
      {0xA, KEY_Z},
      {0xB, KEY_C},
      {0xC, KEY_FOUR},
      {0xD, KEY_R},
      {0xE, KEY_F},
      {0xF, KEY_V}
};

void AudioCB(void *bufferData, unsigned int frames)
{
    i16* d = static_cast<short*>(bufferData);
    static int phase = 0;
    for (u32 i = 0; i < frames; i++) {
        d[i] = ((phase++ / 50) % 2 == 0) ? 8000 : -8000;
    }
}

int main(int argc, char **argv)
{
    if(argc < 2) return 1;
    CPU cpu;
    constexpr int SCALE = 20;

    SetTargetFPS(60);
    InitWindow(64 * SCALE, 32 * SCALE, "CHIP-8");

    InitAudioDevice();
    SetAudioStreamBufferSizeDefault(4096);
    AudioStream beepStream = LoadAudioStream(44100, 16, 1);
    SetAudioStreamCallback(beepStream, AudioCB);

    cpu.loadRom(argv[1]);

    std::array<float, 64 * 32> phosphor{};
    phosphor.fill(.0f);

    constexpr float PHOSPHOR_DECAY = 0.2f;

    constexpr Color BG_COLOUR {5, 20, 5, 255};
    constexpr Color PHOSPHOR_COLOR {51, 255, 0, 255};

    while(!WindowShouldClose()) {

        for(const auto binding : key_bindings)
          cpu.keys[binding.code] = IsKeyDown(binding.binding);

        for(int i = 0; i < 11; i++)
          cpu.run();

        if(cpu.sound_timer > 0) {
            if (!IsAudioStreamPlaying(beepStream))
                PlayAudioStream(beepStream);
        } else {
            if(IsAudioStreamPlaying(beepStream))
                PauseAudioStream(beepStream);
        }

        BeginDrawing();
        ClearBackground(BG_COLOUR);
        for(int i = 0; i < cpu.screen.size(); i++) {
          if(cpu.screen[i]) {
              phosphor[i] = 1.0f;
          } else {
              phosphor[i] -= PHOSPHOR_DECAY;
              if (phosphor[i] < .0f) phosphor[i] = .0f;
          }

          if(phosphor[i] > .0f) {
              int x = i % 64;
              int y = i / 64;

              u8 alpha = static_cast<u8>(phosphor[i] * 255.f);
              Color pixelColour {PHOSPHOR_COLOR};
              pixelColour.a = alpha;
              DrawRectangle(x * SCALE, y * SCALE, SCALE, SCALE, pixelColour);
          }

        }
        EndDrawing();
    }

    UnloadAudioStream(beepStream);
    CloseAudioDevice();
    CloseWindow();

    return 0;
}
