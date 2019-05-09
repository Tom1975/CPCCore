#pragma once

#include "simple_vector.hpp"
#include "simple_string.h"

#define CPCCOREEMU_API 

class SingleElements
{
public:
   const char* filename_;
   unsigned char* buffer_;
   int size_;
};

// Inner files structure : can be either a directory (containing file) or a file.
class NodeFS
{
public:

   NodeFS();
   virtual ~NodeFS();

   NodeFS* InsertLeaf(std::string leaf_name, unsigned char* full_buffer);
   NodeFS* InsertDir(std::string node_name);
   NodeFS* InsertFile(std::string node_name, unsigned char* full_buffer);
   NodeFS* InsertNode(std::string node_name);

   int GetNumberOfElements();
   const char* GetElementName(int& index);
   void AddElementsToList(std::vector<SingleElements>& list, int type);

   int GetType();

   std::string name_;

   bool is_leaf_;

   std::vector<NodeFS*> sub_node_;

   // Element
   unsigned char* full_buffer_;
   unsigned char* Extract();
   unsigned int offset_;
   // Extracted buffer
   unsigned char* buffer_;
   unsigned int size_;
   int type_;
};

/////////////////////////////////////////////////
// Interface TypeManager : can create a value type from any source (filename, inner buffer)
class ITypeManager
{
public:
   virtual int GetTypeFromFile(std::string str) = 0;
   virtual int GetTypeFromBuffer(unsigned char* buffer, int size) = 0;
   virtual int GetTypeFromMultipleTypes(int* type_list, int nb_types) = 0;
};


class IContainedElement
{
public:


   IContainedElement(ITypeManager* type_manager) : type_manager_(type_manager)
   {
   };
   virtual ~IContainedElement() {};
   virtual int GetType() = 0;

   // Number of inner files
   virtual int GetNumberOfElements() = 0;

   // Acess to inner datas
   virtual std::vector<SingleElements> GetInnerElements(int type) = 0;
   virtual const char* GetNameOfElement(int index) = 0;
   virtual unsigned char* GetBuffer(int index) = 0;
   virtual int GetSize(int index) = 0;


protected:
   ITypeManager* type_manager_;
};

class BufferFile : public IContainedElement
{
public:
   BufferFile(ITypeManager* type_manager);
   virtual int GetType() { return false; };
   virtual int GetNumberOfElements() { return 1; };
   virtual std::vector<SingleElements> GetInnerElements(int type);
   // Acess to inner datas
   virtual const char* GetNameOfElement(int index) { return (const char*)nullptr; };

   virtual unsigned char* GetBuffer(int index) { return nullptr; };
   virtual int GetSize(int index) { return 0; };
};

class SingleFile : public IContainedElement
{
public:
   SingleFile(ITypeManager* type_manager, const std::string& file);
   virtual ~SingleFile();

   virtual int GetType();
   virtual int GetNumberOfElements() { return 1; };
   virtual std::vector<SingleElements> GetInnerElements(int type);

   // Acess to inner datas
   virtual const char* GetNameOfElement(int index) { return file_name_.c_str(); };
   virtual unsigned char* GetBuffer(int index);
   virtual int GetSize(int index);

protected:
   // Filename
   std::string file_name_;
   // Buffer
   unsigned char* buffer_;
   int buffer_size_;
};
#ifndef NOZLIB
class ZippedFile : public IContainedElement
{
public:
   ZippedFile(ITypeManager* type_manager, const std::string& file);
   virtual ~ZippedFile();
   virtual int GetType();
   virtual int GetNumberOfElements();
   virtual std::vector<SingleElements> GetInnerElements(int type);

   // Acess to inner datas
   virtual const char* GetNameOfElement(int index);
   virtual unsigned char* GetBuffer(int index);
   virtual int GetSize(int index);

protected:
   void InitUnzip();

   // Filename
   std::string file_name_;
   NodeFS zip_file_;
};
#endif

class DirectoryFile : public IContainedElement
{
public:
   DirectoryFile(ITypeManager* type_manager, const std::string& file);
   virtual int GetType() { return false; };
   virtual int GetNumberOfElements() { return 1; };
   virtual std::vector<SingleElements> GetInnerElements(int type);

   // Acess to inner datas
   virtual const char* GetNameOfElement(int index) { return (const char*)nullptr; };

   virtual unsigned char* GetBuffer(int index) { return nullptr; };
   virtual int GetSize(int index) { return 0; };
};


/////////////////////////////////////////////////
// Container class : contains from 1 to n file. 
// Source cas be :
// - File
// - Directory
// - Zip file

class CPCCOREEMU_API DataContainer
{
public:
   static void Init();


   DataContainer(ITypeManager* type_manager);
   virtual ~DataContainer(void);

   // Clear inner data
   void Clear();

   // Set the container file 
   virtual void AddSourceFile(const char* path);

   const char* GetCurrentPath() { return current_path_.c_str(); }
   // File list
   //bool GetType(ITypeManager *);

   std::vector<IContainedElement*> GetFileList();

   // Get file buffer


   // Type of container : 1 - dsk, 3 - zip
   virtual int GetType(ITypeManager* manager);

   // Inner types functions 


protected:

   void BuildFileList();
   // Private functions
   int GetTypeFromZippedFile(ITypeManager* manager);
   bool BuildFromZippedFile();

   int GetTypeFromDirectory(ITypeManager* manager);
   void BuildFromDirectory();

   int GetTypeFromFile(ITypeManager* manager);
   void BuildFromFile();

   int InitUnzip(ITypeManager* manager, unsigned char* zipped_buffer, int size); // ZIP file


   ////////////////////////////
   // Attributes
   std::string current_path_;
   ITypeManager* type_manager_;
   std::vector<IContainedElement*> file_list_;

   // Main node!
   NodeFS zip_file_;
};
