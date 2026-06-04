#ifndef STACK_MACHINE_HPP
#define STACK_MACHINE_HPP

#include <string>
#include <vector>
#include "runtimeValue.hpp"

struct FrameInfo {
    int base;
    int dynamicLink;
    int returnAddress;
    int staticLink;
};

// Runtime Memory and Stack
class StackMachine {
public:
    static constexpr int FRAME_HEADER_SIZE = 3;

    void reset();

    // INT: allocate slotCount zero-initialized slots in current frame
    void allocateFrame(int slotCount);

    // CAL: push frame header (static link, dynamic link, return address)
    void pushFrame(int returnAddress, int staticLinkLevel);

    // RET: pop current frame, return frame info
    FrameInfo popFrame();

    // Operand stack operations
    void push(const RuntimeValue& value);
    RuntimeValue pop();
    RuntimeValue peek() const;

    // Memory operations
    RuntimeValue load(int address) const;
    void store(int address, const RuntimeValue& value);

    // Resolve address by following static links
    int resolveAddress(int level, int offset) const;

    // Accessors
    int currentBaseAddress() const;
    int memorySize() const;
    bool emptyOperandStack() const;

private:
    std::vector<RuntimeValue> memory;   // Main memory stack (frames + locals)
    std::vector<RuntimeValue> operands; // Expression evaluation stack
    std::vector<FrameInfo> frames;
    int base = 0;  // Current frame base pointer

    void checkMemoryAddress(int address) const;
    void checkOperandAvailable() const;
};

// Helper: convert symbol table address to runtime memory address
inline int runtimeAddressFromSymbolAddress(int symbolAddress) {
    return StackMachine::FRAME_HEADER_SIZE + symbolAddress;
}

#endif
