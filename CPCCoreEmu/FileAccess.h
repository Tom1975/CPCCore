#pragma once

#include "simple_string.h"
#include "simple_vector.hpp"

///////////////////////////////////////////////////
// Generic function : Easier to port to various OS
bool IsDirectory(const char* path);
unsigned int GetDirectoryContent(const char* path, std::vector<std::string>&);
std::string GetFullPath(const char* path);
std::string GetFileFromPath(const char* path);
std::string GetDirectoryFromPath(const char* path);

bool MatchTextWithWildcards(const std::string& text, std::string wildcard_pattern);
void EscapeRegex(std::string& regex);
