#ifndef TESTS_H
#define TESTS_H

#include <cassert>
#include <chrono>
#include <thread>
#include <unistd.h>
#include <iostream>
#include <fstream>
#include "NES.h"
#include "Bus.h"
#include <string>

class Tests {
public:
    void test_cpu();
    void test_opcodes();
    void test_ADC();
    void test_stack();
    void test_reset();
    void test_nmi();
    void test_irq();
    void test_jmp();
    void test_stack_instructions();
    void test_branch();
    void test_ASL();
    void test_LSR();
    void test_ROL();
    void test_ROR();
    void test_CMP();
    void test_CPX();
    void test_CPY();
    void test_CLD_SED_CLV();
    void test_NES(std::string path);
    void test_Bus();
    void test_PPU_registers();
    void test_Pulse1();
};


#endif //TESTS_H
