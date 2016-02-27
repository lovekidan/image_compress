#include "cli/cli.h"
#include <iostream>

static void warn(const boost::program_options::error &e){
	std::cerr << "!!! " << e.what() << " !!!" << std::endl;
}

static int ApngasmCliMain(int argc, char* argv[]){
	try {
		apngasm::APNGAsm apngasm;
		apngasm_cli::CLI cli(argc, argv);
		int res = cli.start();
		return res;
	}
	catch (const boost::program_options::invalid_command_line_syntax &e) {
		warn(e);
		return -1;
	}
	catch (const boost::program_options::unknown_option &e) {
		warn(e);
		return -1;
	}
}

bool ApngCompress(const char* inputFile, const char* outputFile){
    apngasm::APNGAsm assembler;
    assembler.disassemble(std::string(inputFile));
    return assembler.assemble(std::string(outputFile));
}

bool ApngDissemble(const char* inputFile, const char* outputDir){
	if (inputFile == NULL || strlen(inputFile) == 0 || outputDir == NULL || strlen(outputDir) == 0)
		return false;

	char *input = (char *)malloc(strlen(inputFile) + 1);
	strcpy(input, inputFile);
	char *output = (char *)malloc(strlen(outputDir) + 1);
	strcpy(output, outputDir);

	const int argc = 7;
	char* argv[argc];
	argv[0] = "apngasm";
	argv[1] = "-o";
	argv[2] = output;
	argv[3] = "-D";
	argv[4] = input;
	argv[5] = "-x";
	argv[6] = "--force";

    bool res = ApngasmCliMain(argc, argv) == 0;
    free(input);
    free(output);
    return res;
}

bool ApngAssemble(const char *inputDir, const char* outputFile){
	if (inputDir == NULL || strlen(inputDir) == 0 || outputFile == NULL || strlen(outputFile) == 0)
		return false;

	size_t inputDirLength = strlen(inputDir);
	char *xmlFile;
	if (inputDir[inputDirLength - 1] == '\\')
		xmlFile = "animation.xml";
	else
		xmlFile = "\\animation.xml";
	char *input = (char *)malloc(inputDirLength + strlen(xmlFile) + 1);
	strcpy(input, inputDir);
	strcat(input, xmlFile);
	char *output = (char *)malloc(strlen(outputFile) + 1);
	strcpy(output, outputFile);

	const int argc = 6;
	char* argv[argc];
	argv[0] = "apngasm";
	argv[1] = "--file";
	argv[2] = input;
	argv[3] = "-o";
	argv[4] = output;
	argv[5] = "--force";

    bool res = ApngasmCliMain(argc, argv) == 0;
    free(input);
    free(output);
    return res;
}
