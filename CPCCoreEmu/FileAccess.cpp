#include "stdafx.h"
#include "FileAccess.h"
#include <regex>

#ifdef __MORPHOS__
#include <proto/dos.h>
#endif

bool IsDirectory(const char* path)
{
#ifdef __MORPHOS__
    BOOL isDir = FALSE;
    BPTR lock = Lock((path), SHARED_LOCK);

    if(lock != (BPTR)0)
    {
        struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);

        if(fib != NULL)
        {
            if(Examine(lock, fib) == DOSTRUE && fib->fib_DirEntryType >= 0)
            {
                isDir = TRUE;
            }
            FreeDosObject(DOS_FIB, fib);
        }
        UnLock(lock);
    }
    return isDir;
#else
   return (fs::is_directory(fs::status(fs::path(path))));
#endif
}

unsigned int GetDirectoryContent(const char* path, std::vector<std::string>& file_list)
{
   int nb_file = 0;
#ifdef __MORPHOS__
    BPTR dirLock = Lock(path, SHARED_LOCK);
    struct FileInfoBlock *fib = (struct FileInfoBlock *)AllocDosObject(DOS_FIB, NULL);

    // Lock the directory to scan
    if(dirLock && Examine(dirLock, fib))
    {
        // Go through all the file and create the content list
        while(ExNext(dirLock, fib))
        {
            std::string fileName = GetFullPath(fib->fib_FileName);

            if(fib->fib_DirEntryType >= 0)    // Directory
            {
                nb_file += GetDirectoryContent(fileName.c_str(), file_list);
            }
            else if(fib->fib_DirEntryType < 0) // File
            {
                file_list.push_back(fileName.c_str());
            }
            nb_file++;
        }
    }
    UnLock(dirLock);
    FreeDosObject(DOS_FIB, fib);
#else
   for (auto& p : fs::directory_iterator(path))
   {
      if (fs::is_directory(p.status()))
      {
         nb_file += GetDirectoryContent(p.path().string().c_str(), file_list);
      }
      else if (fs::is_regular_file(p.status()))
      {
         // todo
         file_list.push_back(p.path().string());
         nb_file++;
      }
   }
#endif

   return nb_file;
}


std::string GetDirectoryFromPath(const char* path)
{
#ifdef __MORPHOS__
   return std::string(path, PathPart(path) - path);
#else
   fs::path full_path(path);
   full_path.remove_filename();
   return full_path.string();
#endif
}


std::string GetFullPath(const char* path)
{
#ifdef __MORPHOS__
   BPTR lock = Lock(path, SHARED_LOCK);
   TEXT buffer[1024] = "\0"; // Crappy... we should use dynamic allocation... well by design we should not use full path at all :)
   NameFromLock(lock, (STRPTR)buffer, sizeof(buffer));
   UnLock(lock);
   return std::string((char *)buffer);
#else
   fs::path full_path(path);
   return full_path.string();
#endif
}

std::string GetFileFromPath(const char* path)
{
#ifdef __MORPHOS__
    return std::string(FilePart(path));
#else
   fs::path full_path(path);
   return full_path.filename().string();
#endif
}


size_t ReplaceAll(std::string& str, const std::string& from, const std::string& to)
{
   size_t count = 0;

   size_t pos = 0;
   while ((pos = str.find(from, pos)) != std::string::npos)
   {
      str.replace(pos, from.length(), to);
      pos += to.length();
      ++count;
   }

   return count;
}

void EscapeRegex(std::string& regex)
{
   ReplaceAll(regex, "\\", "\\\\");
   ReplaceAll(regex, "^", "\\^");
   ReplaceAll(regex, ".", "\\.");
   ReplaceAll(regex, "$", "\\$");
   ReplaceAll(regex, "|", "\\|");
   ReplaceAll(regex, "(", "\\(");
   ReplaceAll(regex, ")", "\\)");
   ReplaceAll(regex, "[", "\\[");
   ReplaceAll(regex, "]", "\\]");
   ReplaceAll(regex, "*", "\\*");
   ReplaceAll(regex, "+", "\\+");
   ReplaceAll(regex, "?", "\\?");
   ReplaceAll(regex, "/", "\\/");
}

bool MatchTextWithWildcards(const std::string& text, std::string wildcard_pattern)
{
   // Escape all regex special chars
   EscapeRegex(wildcard_pattern);

   // Convert chars '*?' back to their regex equivalents
   ReplaceAll(wildcard_pattern, "\\?", ".");
   ReplaceAll(wildcard_pattern, "\\*", ".*");

   std::regex pattern(wildcard_pattern /*, caseSensitive ? std::regex::normal : std::regex::icase*/);

   return std::regex_match(text, pattern);
}
