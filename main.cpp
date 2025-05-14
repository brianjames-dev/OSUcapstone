#include "tests.h"
#include "string"

#define SDL_MAIN_HANDLED
#include <SDL2/SDL.h>
#include <iostream>

int main(int argc, char* argv[]) {


  std::string testPath;

  for (int i = 0; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "debug") {
      // std::cout << "Debug on!\n";
      // TODO: Update debug mode
    } else if (arg.rfind("test=", 0) == 0) {
      std::string testPath = arg.substr(5);
    }
  }

  if (testPath.empty()) {
    testPath = "./nestest.nes";
  }

	SDL_Init(SDL_INIT_EVERYTHING);
	std::cout << "SDL Initialized Successfully." << std::endl;

	// // TESTS -- uncomment as needed
	Tests tests;
	tests.test_cpu();
	tests.test_opcodes();
	tests.test_stack();
	tests.test_reset();
	tests.test_ADC();
	tests.test_nmi();
	tests.test_irq();
	tests.test_jmp();
	tests.test_stack_instructions();
	tests.test_branch();
	tests.test_ASL();
	tests.test_LSR();
	tests.test_ROL();
	tests.test_ROR();
	tests.test_CMP();
	tests.test_CPX();
	tests.test_CPY();
	tests.test_CLD_SED_CLV();
	// tests.test_NES(testPath);
	tests.test_Bus();
	tests.test_PPU_registers();
	tests.test_Pulse1();
	SDL_Quit();
    return 0;
}

