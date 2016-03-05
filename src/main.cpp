#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>
#include "icutil/ic_util.hpp"
#include <boost/filesystem.hpp>

using std::string;

bool compress_image_from_file(string& inputPath, string& outputPath, int quality) {
	IMAGE_TYPE image_type = ic_get_image_type(inputPath.c_str());
	printf("%s 's type is %d\n", inputPath.c_str(), image_type);
	switch (image_type) {
		case IMAGE_TYPE_NORMAL_PNG:
		ic_compress_png(inputPath.c_str(), outputPath.c_str(), quality);
		break;
		case IMAGE_TYPE_APNG:
		ic_compress_apng(inputPath.c_str(), outputPath.c_str());
		break;
		case IMAGE_TYPE_JPEG:
		ic_compress_jpeg(inputPath.c_str(), outputPath.c_str(), quality);
		break;
		case IMAGE_TYPE_UNCOMPLIED_9PNG:
		ic_compress_nine_patch_png(inputPath.c_str(), outputPath.c_str(), quality);
		break;
	}
	return true;
}

std::vector<string> get_files_from_dir(const string& inputPath) {
	std::vector<string> files;
	boost::filesystem::directory_iterator dirIterBegin(inputPath);
	boost::filesystem::directory_iterator dirIterEnd;
	for (; dirIterBegin != dirIterEnd; ++ dirIterBegin) {
		if (!boost::filesystem::is_directory(*dirIterBegin)) {
			files.push_back(string(dirIterBegin->path().c_str()));
		}
	}
	return files;
}

string combine_file_path(const string& dir, const string& postFix, const string& name, const string& extension) {
	return string().append(dir).append(name).append(postFix).append(extension);
}

bool compress_image_from_dir(string& inputPath, string& outputPath, int quality) {
	std::vector<string> files = get_files_from_dir(inputPath);
	string postFix = 0 != strcmp(inputPath.c_str(), outputPath.c_str()) ? "" : ".compressed";
	string extension = "";
	string baseName = "";
	string outputFilePath = "";
	for (auto iter = files.begin(); files.end() != iter; ++iter) {
		baseName = boost::filesystem::basename(*iter);
		extension = boost::filesystem::extension(*iter);
		outputFilePath = combine_file_path(outputPath, postFix, baseName, extension);
		printf("%s\n", outputPath.c_str());
		compress_image_from_file(*iter, outputFilePath, quality);
	}
	return true;
}

int main(int argc, char** argv) {
	string inputPath;
	string outputPath;
	string errorMessage;
	int quality = 100;
	bool isDir = false;
	if (argc < 2) { // no commands
		goto help;
	}

	for (int i = 1; i < argc; ++i) {
		if (0 == strcmp("-f", argv[i])) { // file
			isDir = false;
			if (i + 1 < argc) {
				inputPath = argv[++i];
				continue;
			} else {
				errorMessage = "you should input a file path after -f.";
				goto error;
			}
		} else if (0 == strcmp("-d", argv[i])) { // directory
			isDir = true;
			if (i + 1 < argc) {
				inputPath = argv[++i];
				continue;
			}  else {
				errorMessage = "you should input a directory path after -d.";
				goto error;
			}
		} else if (0 == strcmp("-o", argv[i])) { // output path
			if (i + 1 < argc) {
				outputPath = argv[++i];
				continue;
			} else {
				errorMessage = "you should input a output path after -o.";
				goto error;
			}
		} else if (0 == strcmp("-q", argv[i])) { // quality
			if (i + 1 < argc) {
				quality = atoi(argv[++i]);
				if (quality < 0 || quality > 100) { // shoule between 0 - 100
					errorMessage = "quality should be a numer between 0 to 100.";
					goto error;
				}
				continue;
			} else {
				errorMessage = "you should input a output path after -o.";
				goto error;
			}
		} else if (0 == strcmp("-help", argv[i])) { // help
			goto help;
		}
	}

	if (0 == inputPath.length()) {
		errorMessage = "invalid command, you can input -help to get right command.";
		goto error;
	}

	if (0 == outputPath.length()) {
		outputPath = inputPath;
	}

	if (isDir) {
		compress_image_from_dir(inputPath, outputPath, quality);
	} else {
		compress_image_from_file(inputPath, outputPath, quality);
	}

	return 0;

	help:
		printf("help for image_compress:\n");
		printf("-f the compress image file path.\n");
		printf("-d the compress image direcotry path.\n");
		printf("-o the output path after compress.\n");
		printf("-q compress quality, a number between 0 to 100.\n");
		printf("example of compress from file: ./image_compress -f filepath -o outputPath -q quality.\n");
		printf("example of compress from directory: ./image_compress -d directory path -o outputPath -q quality.\n");
		return 0;

	error:
		printf("error:%s\n", errorMessage.c_str());
		return -1;
}
