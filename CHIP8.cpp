#include "CHIP8.h"

void CHIP8::Init(const std::string& ROMPath)
{
    curOpcode = 0;
    pc = 0;
    index = 0;
    sp = 0;
    delayTimer = 0;
    soundTimer = 0;
    
    //Load font into memory starting from 0x50
    uint8_t chip8_fontset[80] =
    { 
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

    for (int i = 0; i < 80; i++)
    {
        RAM[i + 0x50] = chip8_fontset[i];
    }
    
    BuildOpCodes();

    LoadROM(ROMPath);

    pc = 512;
}

void CHIP8::LoadROM(const std::string& ROMPath)
{
    std::ifstream file(ROMPath, std::ios::binary);
    FILE* filePtr = fopen(ROMPath.c_str(), "rb");

    if (filePtr != nullptr)
    {
        auto fsize = file.tellg();

        file.seekg(0, std::ios::end);
        fsize = file.tellg() - fsize;
        std::vector<uint8_t> bytes(fsize);

        std::fread(&bytes[0], sizeof(bytes[0]), fsize, filePtr);

        for (int i = 0; i < fsize; i++)
        {
            RAM[i + 0x200] = bytes[i];
        }
    }
    else
    {
        std::cerr << "File failed to open";
    }

    file.close();
    fclose(filePtr);
}

void CHIP8::RunCycle()
{
    std::cerr << "PC at " << pc << '\n';

    //Fetch
    curOpcode = (RAM[pc] << 8) | RAM[pc + 1];

    //Increment pc (need to increment by 2 due to the size of one instruction being 2 bytes)
    //Incrementing before running the opcode avoids altering jump addresses after a cycle
    pc += 2;

    std::cerr << "Running opcode: " << std::hex << curOpcode << '\n';
    try
    {
        opcodeTable[curOpcode]();
    }
    catch(const std::exception& e)
    {
        std::cerr << "Failed to find/run instruction in opcodeTable: " << e.what() << '\n';
    }
}

void CHIP8::BuildOpCodes()
{
    //Instructions with 0x0nnn opcodes
    //CLS
    opcodeTable[0x00E0] = CLS();
    //RET
    opcodeTable[0x00EE] = RET();

    for (int opcode = 0x1000; opcode < 0xFFFF; opcode++)
    {
        uint16_t nnn = opcode & 0x0FFF;
        uint8_t nn = opcode & 0x00FF;
        uint8_t n = opcode & 0x000F;
        uint8_t x = (opcode & 0x0F00) >> 8;
        uint8_t y = (opcode & 0x00F0) >> 4;

        if ((opcode & 0xF000) == 0x1000)
        {
            //JP pc = nnn
            opcodeTable[opcode] = JP_1nnn(nnn);
        }
        else if ((opcode & 0xF000) == 0x2000)
        {
            //CALL pc goes on stack, then pc = nnn
            opcodeTable[opcode] = CALL_2nnn(nnn);
        }
        else if ((opcode & 0xF000) == 0x3000)
        {
            //SE skip if Vx == nn
            opcodeTable[opcode] = SE_3xnn(x, nn);
        }
        else if ((opcode & 0xF000) == 0x4000)
        {
            //SNE skip if Vx != nn
            opcodeTable[opcode] = SNE_4xnn(x, nn);
        }
        else if ((opcode & 0xF000) == 0x5000)
        {
            //SE skip if Vx == Vy
            opcodeTable[opcode] = SE_5xy0(x, y);
        }
        else if ((opcode & 0xF000) == 0x6000)
        {
            //LD Vx = nn
            opcodeTable[opcode] = LD_6xnn(x, nn);
        }
        else if ((opcode & 0xF000) == 0x7000)
        {
            //ADD Vx += nn
            opcodeTable[opcode] = ADD_7xnn(x, nn);
        }
        //0x8--- instructions
        else if ((opcode & 0xF000) == 0x8000)
        {
            if (n == 0)
            {
                //LD Vx = Vy
                opcodeTable[opcode] = LD_8xy0(x, y);
            }
            else if (n == 1)
            {
                //OR Vx || Vy
                opcodeTable[opcode] = OR_8xy1(x, y);
            }
            else if (n == 2)
            {
                //AND Vx & Vy
                opcodeTable[opcode] = AND_8xy2(x, y);
            }
            else if (n == 3)
            {
                //XOR Vx ^ Vy
                opcodeTable[opcode] = XOR_8xy3(x, y);
            }
            else if (n == 4)
            {
                //ADD Vx += Vy and set VF = carry
                opcodeTable[opcode] = ADD_8xy4(x, y);
            }
            else if (n == 5)
            {
                //SUB Vx -= Vy and set VF = carry
                opcodeTable[opcode] = SUB_8xy5(x, y);
            }
            else if (n == 6)
            {
                //SHR (shift right) Vx = Vx >> 1; store LSB of Vx in VF and then divides Vx / 2
                opcodeTable[opcode] = SHR_8xy6(x, y);
            }
            else if (n == 7)
            {
                //SUBN Vx = Vy - Vx and set VF = !(borrow)
                opcodeTable[opcode] = SUBN_8xy7(x, y);
            }
            else if (n == 14)
            {
                //SHL Vx = Vx << 1; store LSB of Vx in VF, then multiply Vx * 2
                opcodeTable[opcode] = SHL_8xyE(x, y);
            }
        }
        else if ((opcode & 0xF00F) == 0x9000)
        {
            //SNE skip if Vx != Vy
            opcodeTable[opcode] = SNE_9xy0(x, y);
        }
        else if ((opcode & 0xF000) == 0xA000)
        {
            //LD I = nnn
            opcodeTable[opcode] = LD_Annn(nnn);
        }
        else if ((opcode & 0xF000) == 0xB000)
        {
            //JP pc = nnn + V0
            opcodeTable[opcode] = JP_Bnnn(nnn);
        }
        else if ((opcode & 0xF000) == 0xC000)
        {
            //RND generate random number [0, 255], then Vx = rng & nn
            opcodeTable[opcode] = RND_Cxnn(x, nn);
        }
        else if ((opcode & 0xF000) == 0xD000)
        {
            //DRW_Dxyn draw(Vx, Vy, n)
            opcodeTable[opcode] = DRW_Dxyn(x, y, n);
        }
        else if ((opcode & 0xF0FF) == 0xE09E)
        {
            //SKP skip if the key with the value in Vx is pressed
            opcodeTable[opcode] = SKP_Ex9E(x);
        }
        else if ((opcode & 0xF0FF) == 0xE0A1)
        {
            //SKNP skip if the key with the value in Vx is not pressed
            opcodeTable[opcode] = SKNP_ExA1(x);
        }
        //0xF--- instructions
        else if ((opcode & 0xF000) == 0xF000)
        {
            if (nn == 0x07)
            {
                //LD Vx = DT (the delay timer value)
                opcodeTable[opcode] = LD_Fx07(x);
            }
            else if (nn == 0x0A)
            {
                //LD Vx = value of pressed key (everything waits until a key is pressed)
                opcodeTable[opcode] = LD_Fx0A(x);
            }
            else if (nn == 0x15)
            {
                //LD DT = Vx
                opcodeTable[opcode] = LD_Fx15(x);
            }
            else if (nn == 0x18)
            {
                //LD ST = Vx
                opcodeTable[opcode] = LD_Fx18(x);
            }
            else if (nn == 0x1E)
            {
                //ADD index += Vx
                opcodeTable[opcode] = ADD_Fx1E(x);
            }
            else if (nn == 0x29)
            {
                //LD I = memory location for the font sprite of Vx
                opcodeTable[opcode] = LD_Fx29(x);
            }
            else if (nn == 0x33)
            {
                //LD store binary-coded decimal representation of Vx (RAM[index] = hundreds, RAM[index+1] = tens, RAM[index+2] = ones)
                opcodeTable[opcode] = LD_Fx33(x);
            }
            else if (nn == 0x55)
            {
                //LD set RAM[index] through RAM[index + x] = V0 through Vx
                opcodeTable[opcode] = LD_Fx55(x);
            }
            else if (nn == 0x65)
            {
                //LD set registers V0 through Vx = RAM[index] through RAM[index + x]
                opcodeTable[opcode] = LD_Fx65(x);
            }
        }
    }
}

CHIP8::Instruction CHIP8::CLS()
{
    return [this]() {
        std::fill(display.begin(), display.end(), 0);
        drawFlag = true;
    };
}

CHIP8::Instruction CHIP8::RET()
{
    return [this]()
    {
        pc = stack[sp];
        sp--;
    };
}

CHIP8::Instruction CHIP8::JP_1nnn(uint16_t nnn)
{
    return [this, nnn]()
    {
        pc = nnn;
    };
}

CHIP8::Instruction CHIP8::CALL_2nnn(uint16_t nnn)
{
    return [this, nnn]()
    {
        sp++;
        stack[sp] = pc;
        pc = nnn;
    };
}

CHIP8::Instruction CHIP8::SE_3xnn(uint8_t x, uint8_t nn)
{
    return [this, x, nn]()
    {
        if (registers[x] == nn)
        {
            pc += 2;
        }
    };
}

CHIP8::Instruction CHIP8::SNE_4xnn(uint8_t x, uint8_t nn)
{
    return [this, x, nn]()
    {
        if (registers[x] != nn)
        {
            pc += 2;
        }
    };
}

CHIP8::Instruction CHIP8::SE_5xy0(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        if (registers[x] == registers[y])
        {
            pc += 2;
        }
    };
}

CHIP8::Instruction CHIP8::LD_6xnn(uint8_t x, uint8_t nn)
{
    return [this, x, nn]()
    {
        registers[x] = nn;
    };
}

CHIP8::Instruction CHIP8::ADD_7xnn(uint8_t x, uint8_t nn)
{
    return [this, x, nn]()
    {
        registers[x] += nn;
    };
}
    
CHIP8::Instruction CHIP8::LD_8xy0(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[x] = registers[y];
    };
}

CHIP8::Instruction CHIP8::OR_8xy1(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[x] |= registers[y];
    };
}

CHIP8::Instruction CHIP8::AND_8xy2(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[x] &= registers[y];
    };
}

CHIP8::Instruction CHIP8::XOR_8xy3(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[x] ^= registers[y];  
    };
}

//Two ADD_8xy4 - the first uses a uint16_t to check if sum > 256, the second uses a different method

CHIP8::Instruction CHIP8::ADD_8xy4(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[0xF] = 0;

        uint16_t result = registers[x] + registers[y];
        registers[x] += registers[y];

        if (result > 0xFF)
        {
            registers[0xF] = 1;
        }
    };
}
// CHIP8::Instruction CHIP8::ADD_8xy4(uint8_t x, uint8_t y)
// {
//     return [this, x, y]()
//     {
//         registers[0xF] = 0;
//
//         if (registers[x] > (0xFF - registers[y]))
//         {
//             registers[0xF] = 1;
//         }
//
//         registers[x] += registers[y];
//     };
// }

CHIP8::Instruction CHIP8::SUB_8xy5(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[0xF] = 0;

        if (registers[x] > registers[y])
        {
            registers[0xF] = 1;
        }

        registers[x] = registers[x] - registers[y];
    };
}

CHIP8::Instruction CHIP8::SHR_8xy6(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[0xF] = 0;

        if ((registers[x] & 0x01) > 0)
        {
            registers[0xF] = 1;
        }

        registers[x] = registers[x] >> 1;
    };
}

CHIP8::Instruction CHIP8::SUBN_8xy7(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[0xF] = 0;

        if (registers[x] < registers[y])
        {
            registers[0xF] = 1;
        }

        registers[x] = registers[y] - registers[x];
    };
}

CHIP8::Instruction CHIP8::SHL_8xyE(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        registers[0xF] = 0;
        
        if ((registers[x] & 0x80) > 0)
        {
            registers[0xF] = 1;
        }

        registers[x] = registers[x] << 1;
    };
}

CHIP8::Instruction CHIP8::SNE_9xy0(uint8_t x, uint8_t y)
{
    return [this, x, y]()
    {
        if (registers[x] != registers[y])
        {
            pc += 2;
        }
    };
}

CHIP8::Instruction CHIP8::LD_Annn(uint16_t nnn)
{
    return [this, nnn]()
    {
        index = nnn;
    };
}

CHIP8::Instruction CHIP8::JP_Bnnn(uint16_t nnn)
{
    return [this, nnn]()
    {
        pc = (nnn + registers[0]);
    };
}

CHIP8::Instruction CHIP8::RND_Cxnn(uint8_t x, uint8_t nn)
{
    return [this, x, nn]()
    {
        uint8_t randNum = std::rand() % 255;

        registers[x] = randNum & nn;
    };
}

//Draw sprite 8 pixels wide and n high at (Vx, Vy) using a sprite from the address the I register points to
//x and y indicate which registers to use, n determines how many rows high the sprite is (and therefore how many bytes to read from I)
//The display pixel vector uses 1 uint8_t for each pixel, while the bytes loaded from the index register use 1 bit per pixel
CHIP8::Instruction CHIP8::DRW_Dxyn(uint8_t x, uint8_t y, uint8_t n)
{
    return [this, x, y ,n]()
    {
        //Get the coordinates from Vx and Vy (and adjust if they are off the screen)
        uint8_t xCoordinate = registers[x]; //% 64;
        uint8_t yCoordinate = registers[y]; //% 32;
        uint8_t curPixelByte;

        //Set register VF to 0 - it will be set to 1 if any pixels overlap and are both on
        registers[0xF] = 0;

        //Loop through n rows of 8 pixels
        for (int i = 0; i < n; i++)
        {
            curPixelByte = RAM[index + i];

            for (int j = 0; j < 8; j++)
            { 
                //AND the byte from index with a bitmask to check if this bit is 1
                if ((curPixelByte & (0b10000000 >> j)) != 0)
                {
                    //Set VF=1 if pixel[bit j] AND display are both 1
                    if (display[(((yCoordinate + i) % 32) * 64) + ((xCoordinate + j) % 64)] == 1)
                    {
                        registers[0xF] = 1;
                    }

                    display[(((yCoordinate + i) % 32) * 64) + ((xCoordinate + j) % 64)] ^= 1;
                }
            }
        }

        drawFlag = true;    
    };
}
    
CHIP8::Instruction CHIP8::SKP_Ex9E(uint8_t x)
{
    return [this, x]()
    {
        if (keyboardState[registers[x]] == 1)
        {
            pc += 2;
        }
    };
}

CHIP8::Instruction CHIP8::SKNP_ExA1(uint8_t x)
{
    return [this, x]()
    {
        if (keyboardState[registers[x]] == 0)
        {
            pc += 2;
        }
    };
}

CHIP8::Instruction CHIP8::LD_Fx07(uint8_t x)
{
    return [this, x]()
    {
        registers[x] = delayTimer;
    };
}

CHIP8::Instruction CHIP8::LD_Fx0A(uint8_t x)
{
    return [this, x]()
    {
        uint8_t keyVal = 0;
        bool keyPressed = false;
        auto IsZero = [](uint8_t i){ return i == 0; };

        while (keyPressed)
        {
            auto pressedKey = std::find_if_not(keyboardState.begin(), keyboardState.end(), IsZero);

            if (pressedKey != keyboardState.end())
            {
                keyVal = *pressedKey;
                keyPressed = true;
            }

            registers[x] = keyVal;
        }
    };
}

CHIP8::Instruction CHIP8::LD_Fx15(uint8_t x)
{
    return [this, x]()
    {
        delayTimer = registers[x];
    };
}

CHIP8::Instruction CHIP8::LD_Fx18(uint8_t x)
{
    return [this, x]()
    {
        registers[x] = soundTimer;
    };
}

CHIP8::Instruction CHIP8::ADD_Fx1E(uint8_t x)
{
    return [this, x]()
    {
        index += registers[x];
    };
}

CHIP8::Instruction CHIP8::LD_Fx29(uint8_t x)
{
    return [this, x]()
    {
        index = 0x50 + (registers[x] * 5);
    };
}

CHIP8::Instruction CHIP8::LD_Fx33(uint8_t x)
{
    return [this, x]()
    {
        RAM[index] = registers[x] / 100;
        RAM[index + 1] = (registers[x] % 100) / 10;
        RAM[index + 2] = (registers[x] % 100) % 10;
    };
}

CHIP8::Instruction CHIP8::LD_Fx55(uint8_t x)
{
    return [this, x]()
    {
        for (uint8_t i = 0; i <= x; i++)
        {
            RAM[index + i] = registers[i];
        }
    };
}

CHIP8::Instruction CHIP8::LD_Fx65(uint8_t x)
{
    return [this, x]()
    {
        for (uint8_t i = 0; i <= x; i++)
        {
            registers[i] = RAM[index + i];
        }
    };
}