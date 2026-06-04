#include "stackMachine.hpp"
#include <stdexcept>
#include <iostream>

void StackMachine::reset() {
    memory.clear();
    operands.clear();
    frames.clear();
    base = 0;
}

void StackMachine::allocateFrame(int slotCount) {
    for (int i = 0; i < slotCount; i++) {
        memory.push_back(RuntimeValue(0));
    }
}

void StackMachine::pushFrame(int returnAddress, int staticLinkLevel) {
    // Find static link: follow static links from current base
    int staticLink = base;
    for (int i = 0; i < staticLinkLevel; i++) {
        if (staticLink >= 0 && staticLink < (int)memory.size()) {
            staticLink = std::get<int>(memory[staticLink].value);
        } else {
            staticLink = 0;
            break;
        }
    }

    // Save current frame info
    FrameInfo fi;
    fi.base = base;
    fi.dynamicLink = base;  // Dynamic link = previous frame base
    fi.returnAddress = returnAddress;
    fi.staticLink = staticLink;
    frames.push_back(fi);

    // Push frame header to memory
    memory.push_back(RuntimeValue(staticLink));  // [base+0]: Static Link
    memory.push_back(RuntimeValue(base));         // [base+1]: Dynamic Link
    memory.push_back(RuntimeValue(returnAddress)); // [base+2]: Return Address

    // Set new base to point to start of frame header (static link position)
    base = (int)memory.size() - 3;
}

FrameInfo StackMachine::popFrame() {
    if (frames.empty()) {
        // Main program termination — no frame header was pushed (INT only allocated slots).
        // Return a sentinel with returnAddress = -1 to signal the interpreter to stop.
        FrameInfo info;
        info.base = base;
        info.dynamicLink = 0;
        info.returnAddress = -1;
        info.staticLink = 0;
        reset();
        return info;
    }

    FrameInfo info = frames.back();
    frames.pop_back();

    // Pop everything from current frame (header + locals + temporaries)
    int frameStart = base;
    int frameSize = (int)memory.size() - frameStart;
    for (int i = 0; i < frameSize; i++) {
        memory.pop_back();
    }

    // Restore base to previous frame
    base = info.base;

    return info;
}

void StackMachine::push(const RuntimeValue& value) {
    operands.push_back(value);
}

RuntimeValue StackMachine::pop() {
    if (operands.empty()) {
        throw std::runtime_error("Operand stack underflow");
    }
    RuntimeValue v = operands.back();
    operands.pop_back();
    return v;
}

RuntimeValue StackMachine::peek() const {
    if (operands.empty()) {
        throw std::runtime_error("Operand stack empty");
    }
    return operands.back();
}

RuntimeValue StackMachine::load(int address) const {
    if (address < 0 || address >= (int)memory.size()) {
        throw std::runtime_error("Memory access violation at address " + std::to_string(address));
    }
    return memory[address];
}

void StackMachine::store(int address, const RuntimeValue& value) {
    if (address < 0 || address >= (int)memory.size()) {
        throw std::runtime_error("Memory access violation at address " + std::to_string(address));
    }
    memory[address] = value;
}

int StackMachine::currentBaseAddress() const {
    return base;
}

int StackMachine::memorySize() const {
    return (int)memory.size();
}

bool StackMachine::emptyOperandStack() const {
    return operands.empty();
}

int StackMachine::resolveAddress(int level, int offset) const {
    int addr = base;
    for (int i = 0; i < level; i++) {
        if (addr >= 0 && addr < (int)memory.size()) {
            addr = std::get<int>(memory[addr].value);  // Follow static link
        } else {
            addr = 0;
            break;
        }
    }
    return addr + offset;
}
