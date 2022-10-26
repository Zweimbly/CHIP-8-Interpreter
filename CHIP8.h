#include <SDL2/SDL.h>
#include <fstream>
#include <iostream>
#include <unordered_map>
#include <vector>
#include <functional>
#include <chrono>
#include <thread>

class CHIP8
{
public:
    //Init with path to ROM file
    void Init(const std::string& ROMPath);

    void RunCycle();

    std::vector<uint8_t> display = std::vector<uint8_t>(64 * 32, 0);
    bool drawFlag = false;

    std::vector<uint8_t> keyboardState = std::vector<uint8_t>(16, 0);

    //CPU cycle frequency target in milliseconds - ex. 0.5ms is 2k cycles/second, 1 cycle every 0.0005 seconds
    //500Hz is 1 cycle / 2ms, and 60Hz is 1 cycle / 16.67ms
    //At 500Hz and updating everything at 60Hz, you would run 8 CPU cycles per update cycle
    int cyclesPerUpdate = 8;

	uint8_t delayTimer;

	uint8_t soundTimer;

private:
    //Load ROM
    void LoadROM(const std::string& ROMPath);
    
    //Build the instruction set
    void BuildOpCodes();

    //Instruction set done 3 ways: hash table, array, vector
    //std::unordered_map<uint16_t, std::function<void(void)>> opcodeTable;
    //std::function<void(void)> opcodeTable[0xFFFF];
    std::vector<std::function<void(void)>> opcodeTable = std::vector<std::function<void(void)>>(0xFFFF);

    uint16_t curOpcode;
    
    //4kb Memory
	std::vector<uint8_t> RAM = std::vector<uint8_t>(4096, 0);
    int maxROMSize = 0xFFF;

	//16 one-byte registers
	std::vector<uint8_t> registers = std::vector<uint8_t>(16, 0x00);

	//Program Counter
	uint16_t pc;

	//Index Register
	uint16_t index;

	//Stack of 16-bit addresses
	std::vector<uint16_t> stack = std::vector<uint16_t>(16, 0x00);

	//Stack pointer
	uint8_t sp;

    //Instruction set functions - abbreviation followed by op # and arguments (eg., 0x1nnn for JP is JP_1nnn)
    using Instruction = std::function<void(void)>;
    Instruction CLS();
    Instruction RET();
    Instruction JP_1nnn(uint16_t nnn);
    Instruction CALL_2nnn(uint16_t nnn);
    Instruction SE_3xnn(uint8_t x, uint8_t nn);
    Instruction SNE_4xnn(uint8_t x, uint8_t nn);
    Instruction SE_5xy0(uint8_t x, uint8_t y);
    Instruction LD_6xnn(uint8_t x, uint8_t nn);
    Instruction ADD_7xnn(uint8_t x, uint8_t nn);
    Instruction LD_8xy0(uint8_t x, uint8_t y);
    Instruction OR_8xy1(uint8_t x, uint8_t y);
    Instruction AND_8xy2(uint8_t x, uint8_t y);
    Instruction XOR_8xy3(uint8_t x, uint8_t y);
    Instruction ADD_8xy4(uint8_t x, uint8_t y);
    Instruction SUB_8xy5(uint8_t x, uint8_t y);
    Instruction SHR_8xy6(uint8_t x, uint8_t y);
    Instruction SUBN_8xy7(uint8_t x, uint8_t y);
    Instruction SHL_8xyE(uint8_t x, uint8_t y);
    Instruction SNE_9xy0(uint8_t x, uint8_t y);
    Instruction LD_Annn(uint16_t nnn);
    Instruction JP_Bnnn(uint16_t nnn);
    Instruction RND_Cxnn(uint8_t x, uint8_t nn);
    Instruction DRW_Dxyn(uint8_t x, uint8_t y, uint8_t n);
    Instruction SKP_Ex9E(uint8_t x);
    Instruction SKNP_ExA1(uint8_t x);
    Instruction LD_Fx07(uint8_t x);
    Instruction LD_Fx0A(uint8_t x);
    Instruction LD_Fx15(uint8_t x);
    Instruction LD_Fx18(uint8_t x);
    Instruction ADD_Fx1E(uint8_t x);
    Instruction LD_Fx29(uint8_t x);
    Instruction LD_Fx33(uint8_t x);
    Instruction LD_Fx55(uint8_t x);
    Instruction LD_Fx65(uint8_t x);
};