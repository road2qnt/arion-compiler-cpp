#ifndef STACK_MACHINE_HPP
#define STACK_MACHINE_HPP

#include <string>
#include <vector>
#include "runtimeValue.hpp"

struct StackFrame {
    int staticLink;
    int dynamicLink;
    int returnAddress;
    int baseAddress;
    int size;
};

// Runtime Memory and Stack
class StackMachine {
public:
    static constexpr int FRAME_HEADER_SIZE = 3;

    void reset();
    void allocateFrame(int slotCount);
    void pushFrame(int returnAddress, int localSlotCount);
    void popFrame();

    void push(const RuntimeValue& value);
    RuntimeValue pop();
    RuntimeValue peek() const;

    RuntimeValue load(int address) const;
    void store(int address, const RuntimeValue& value);

    int currentBaseAddress() const;
    int memorySize() const;
    bool emptyOperandStack() const;

private:
    std::vector<RuntimeValue> memory;
    std::vector<RuntimeValue> operands;
    std::vector<StackFrame> frames;

    void checkMemoryAddress(int address) const;
    void checkOperandAvailable() const;
};

// Offset (3)
inline int runtimeAddressFromSymbolAddress(int symbolAddress) {
    return StackMachine::FRAME_HEADER_SIZE + symbolAddress;
}

#endif
