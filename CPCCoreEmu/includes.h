#pragma once 
//////////////////////////////////////////////////////////
// includes.h
//
// standard includes and adapter
//
//
//////////////////////////////////////////////////////////


//////////////////////////////////////////////////////////
// filesystem usage : only for C++17
// NOTE : For c++ <17, try using boost::filesystem (which is roughly the same)

#ifdef _WIN32
#include <experimental\filesystem>
namespace fs = std::experimental::filesystem;
#elif __unix
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;
#endif
