#include "stdafx.h"

#include "DiskContainer.h"
#include "FileAccess.h"
#include "simple_stdio.h"

#define  NOZLIB

#ifndef NOZLIB
#include "zlib.h"

#define FILE_ATTRIBUTE_DIRECTORY            0x00000010
#define INVALID_FILE_ATTRIBUTES ((DWORD)-1)

/////////////////////////////////////////////////////////////////
// Node  Implementation
/////////////////////////////////////////////////////////////////
NodeFS::NodeFS() : buffer_(nullptr), offset_(0), type_(-1)
{
}

NodeFS::~NodeFS()
{
   for (auto& it : sub_node_)
   {
      delete it;
   }

   delete[]buffer_;
}

void NodeFS::AddElementsToList(std::vector<SingleElements>& list, int type)
{
   if (type_ == type)
   {
      SingleElements element;
      element.filename_ = name_.c_str();
      element.buffer_ = buffer_;
      element.size_ = size_;

      list.push_back(element);
   }

   for (std::vector<NodeFS*>::iterator it = sub_node_.begin(); it != sub_node_.end(); ++it)
   {
      (*it)->AddElementsToList(list, type);
   }
}

int NodeFS::GetType()
{
   if (type_ != -1) return type_;
   for (std::vector<NodeFS*>::iterator it = sub_node_.begin(); it != sub_node_.end(); ++it)
   {
      int type_node = (*it)->GetType();
      if (type_node != -1)
         return type_node;
   }
   return 0;
}

const char* NodeFS::GetElementName(int& index)
{
   if (type_ != -1)
   {
      if (index == 0)
      {
         return name_.c_str();
      }
      index--;
   }

   for (std::vector<NodeFS*>::iterator it = sub_node_.begin(); it != sub_node_.end(); ++it)
   {
      const char* filename_out = (*it)->GetElementName(index);
      if (filename_out != nullptr)
      {
         return filename_out;
      }
   }
   return nullptr;
}

int NodeFS::GetNumberOfElements()
{
   int nb_elements = 0;
   if (type_ != -1) nb_elements++;
   for (std::vector<NodeFS*>::iterator it = sub_node_.begin(); it != sub_node_.end(); ++it)
   {
      nb_elements += (*it)->GetNumberOfElements();
   }
   return nb_elements;
}

NodeFS* NodeFS::InsertLeaf(std::string leaf_name, unsigned char* full_buffer)
{
   NodeFS* new_leaf = new NodeFS;
   new_leaf->full_buffer_ = full_buffer;
   new_leaf->name_ = leaf_name;
   new_leaf->is_leaf_ = true;
   sub_node_.push_back(new_leaf);
   return new_leaf;
}

NodeFS* NodeFS::InsertNode(std::string node_name)
{
   // Look for node : Already exists ?
   for (std::vector<NodeFS*>::iterator it = sub_node_.begin(); it != sub_node_.end(); ++it)
   {
      if ((*it)->name_ == node_name)
         return *it;
   }
   // It does not exists
   NodeFS* new_node = new NodeFS;
   new_node->name_ = node_name;
   new_node->is_leaf_ = false;
   sub_node_.push_back(new_node);

   return new_node;
}

NodeFS* NodeFS::InsertDir(std::string node_name)
{
   // Parse the name to get the complete tree
   int pos = node_name.find('/', 0);
   if (pos != std::string::npos)
   {
      // Subdirectory inside :
      NodeFS* node = InsertNode(node_name.substr(0, pos));
      // Add new node (if necessary) then recurse
      return node->InsertDir(node_name.substr(pos + 1));
   }
   if (node_name.size() > 0)
   {
      // No more : Add it to the list of directories
      // Subdirectory inside :
      return InsertNode(node_name.substr(0, pos - 1));
   }
   return nullptr;
}

NodeFS* NodeFS::InsertFile(std::string node_name, unsigned char* full_buffer)
{
   // Parse the name to get the complete tree
   int pos = node_name.find('/', 0);
   if (pos != std::string::npos)
   {
      // Subdirectory inside :
      NodeFS* node = InsertNode(node_name.substr(0, pos));

      // Add new node (if necessary) then recurse
      return node->InsertFile(node_name.substr(pos + 1), full_buffer);
   }
   if (node_name.size() > 0)
   {
      // Check if this name already exists
      // todo

      // No more : Add it to the list of directories
      // Subdirectory inside :
      return InsertLeaf(node_name.substr(0, pos - 1), full_buffer);
   }
   return nullptr;
}


unsigned char* NodeFS::Extract()
{
   if (!is_leaf_) return nullptr;

   short min_version = *(short*)&(full_buffer_[offset_ + 4]);
   short comp_method = *(short*)&(full_buffer_[offset_ + 8]);
   int compressed_size = *(int*)&(full_buffer_[offset_ + 18]);
   int uncompressed_size = *(int*)&(full_buffer_[offset_ + 22]);
   short n = *(short*)&(full_buffer_[offset_ + 26]);
   short m = *(short*)&(full_buffer_[offset_ + 28]);

   if (min_version > 0x14 ||
      (comp_method != 0 && comp_method != 8)
   )
      return nullptr;

   // Ok, then extract !
   if (buffer_ != nullptr) delete[]buffer_;
   buffer_ = new unsigned char[uncompressed_size];
   size_ = uncompressed_size;

   // zlib struct
   z_stream defstream;
   defstream.zalloc = nullptr;
   defstream.zfree = nullptr;
   defstream.opaque = nullptr;
   // setup "a" as the input and "b" as the compressed output
   defstream.avail_in = (uInt)compressed_size; // size of input, string + terminator
   defstream.next_in = &full_buffer_[offset_ + 30 + n + m]; // input char array
   defstream.avail_out = uncompressed_size; // size of output
   defstream.next_out = buffer_; // output char array

   // the actual compression work.
   inflateInit2(&defstream, -MAX_WBITS);
   inflate(&defstream, Z_NO_FLUSH);
   inflateEnd(&defstream);

   return buffer_;
}


/////////////////////////////////////////////////////////////////
// SingleFile Implementation
/////////////////////////////////////////////////////////////////
BufferFile::BufferFile(ITypeManager* type_manager): IContainedElement(type_manager)
{
}

std::vector<SingleElements> BufferFile::GetInnerElements(int type)
{
   std::vector<SingleElements> list;
   return list;
}
#endif


/////////////////////////////////////////////////////////////////
// SingleFile Implementation
/////////////////////////////////////////////////////////////////
SingleFile::SingleFile(ITypeManager* type_manager, const std::string& file) : IContainedElement(type_manager),
                                                                              file_name_(file), buffer_(nullptr),
                                                                              buffer_size_(0)
{
   // Read file
   FILE* f;
   if (fopen_s(&f, file_name_.c_str(), "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      buffer_size_ = ftell(f);
      rewind(f);
      buffer_ = new unsigned char[buffer_size_];

      fread(buffer_, buffer_size_, 1, f);
      fclose(f);
   }
}

SingleFile::~SingleFile()
{
   //
   delete[]buffer_;
}

int SingleFile::GetType()
{
   // Check from buffer
   int type = type_manager_->GetTypeFromBuffer(buffer_, buffer_size_);
   if (type == 0)
      return type_manager_->GetTypeFromFile(file_name_.c_str());
   return type;
}

unsigned char* SingleFile::GetBuffer(int index)
{
   return buffer_;
}

int SingleFile::GetSize(int index)
{
   return buffer_size_;
}

std::vector<SingleElements> SingleFile::GetInnerElements(int type)
{
   std::vector<SingleElements> list;
   SingleElements element;
   element.filename_ = file_name_.c_str();
   element.buffer_ = buffer_;
   element.size_ = buffer_size_;

   list.push_back(element);
   return list;
}

#ifndef NOZLIB

/////////////////////////////////////////////////////////////////
// ZippedFile Implementation
/////////////////////////////////////////////////////////////////

ZippedFile::ZippedFile(ITypeManager* type_manager, const std::string& file) : IContainedElement(type_manager),
                                                                              file_name_(file)
{
   // Unzip to file
   InitUnzip();
}

ZippedFile::~ZippedFile()
{
   // Clear
}

std::vector<SingleElements> ZippedFile::GetInnerElements(int type)
{
   std::vector<SingleElements> list;

   int nb_elements = GetNumberOfElements();
   for (std::vector<NodeFS*>::iterator it = zip_file_.sub_node_.begin(); it != zip_file_.sub_node_.end(); it++)
   {
      (*it)->AddElementsToList(list, type);
   }

   return list;
}

unsigned char* ZippedFile::GetBuffer(int index)
{
   // Return first buffer of first file found
   for (std::vector<NodeFS*>::iterator it = zip_file_.sub_node_.begin(); it != zip_file_.sub_node_.end();)
   {
      // Is it a disk ?
      return (*it)->buffer_;
   }
   return nullptr;
}

int ZippedFile::GetSize(int index)
{
   for (std::vector<NodeFS*>::iterator it = zip_file_.sub_node_.begin(); it != zip_file_.sub_node_.end();)
   {
      // Is it a disk ?
      return (*it)->size_;
   }
   return 0;
}

int ZippedFile::GetType()
{
   // Extract files
   int ret = 0;

   // Return first buffer of first file found
   for (std::vector<NodeFS*>::iterator it = zip_file_.sub_node_.begin(); it < zip_file_.sub_node_.end(); it++)
   {
      // Is it a disk ?
      int type = (*it)->GetType();

      if (type > 0)
         return type;
   }
   return ret;
}

int ZippedFile::GetNumberOfElements()
{
   int nb_elements = 0;
   for (std::vector<NodeFS*>::iterator it = zip_file_.sub_node_.begin(); it < zip_file_.sub_node_.end(); it++)
   {
      // Is it a disk ?
      nb_elements += (*it)->GetNumberOfElements();
   }
   return nb_elements;
}

// Acess to inner datas
const char* ZippedFile::GetNameOfElement(int index)
{
   for (std::vector<NodeFS*>::iterator it = zip_file_.sub_node_.begin(); it < zip_file_.sub_node_.end(); it++)
   {
      const char* element_name = (*it)->GetElementName(index);

      // Is it a disk ?
      if (element_name != nullptr)
      {
         return element_name;
      }
   }
   return (const char*)nullptr;
}

void ZippedFile::InitUnzip()
{
   // Open zipped content, then check it
   FILE* f;
   if (fopen_s(&f, file_name_.c_str(), "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      long size = ftell(f);
      rewind(f);
      unsigned char* zipped_buffer = new unsigned char[size];

      fread(zipped_buffer, size, 1, f);
      fclose(f);

      if (*(unsigned int*)zipped_buffer == 0x04034b50)
      {
         int offset_eocd = -1;
         for (int offset = size - 1; offset >= 0; offset--)
         {
            if (*(unsigned int*)(&zipped_buffer[offset]) == 0x06054B50)
            {
               offset_eocd = offset;
               break;
            }
         }
         if (offset_eocd != -1)
         {
            // Found : Get infos.
            int nb_central_dir = *(short*)&(zipped_buffer[offset_eocd + 8]);
            int offset_dir = *(int*)&(zipped_buffer[offset_eocd + 16]);

            // Now, look at central directories
            for (int cd = 0; cd < nb_central_dir; cd++)
            {
               // Check signature
               if (*(unsigned int*)(&zipped_buffer[offset_dir]) == 0x02014b50)
               {
                  // check version min
                  // Compression method
                  short min_version = *(short*)&(zipped_buffer[offset_dir + 6]);
                  short comp_method = *(short*)&(zipped_buffer[offset_dir + 10]);
                  short ifile_attrib = *(short*)&(zipped_buffer[offset_dir + 36]);
                  int efile_attrib = *(int*)&(zipped_buffer[offset_dir + 38]);

                  //
                  int off_local_header = *(int*)&(zipped_buffer[offset_dir + 42]);
                  // Sizes
                  int filename_size = *(short*)(&zipped_buffer[offset_dir + 28]);
                  int extra_size = *(short*)(&zipped_buffer[offset_dir + 30]);
                  int Comment_size = *(short*)(&zipped_buffer[offset_dir + 32]);
                  char* filename = new char[filename_size + 1];
                  memcpy(filename, &zipped_buffer[offset_dir + 46], filename_size);
                  filename[filename_size] = 0;

                  // Directory ?
                  NodeFS* added_element;
                  if (efile_attrib == FILE_ATTRIBUTE_DIRECTORY)
                  {
                     // New node :
                     // CONVERT TO char
                     added_element = zip_file_.InsertDir(filename);
                  }
                  else
                  {
                     // File

                     added_element = zip_file_.InsertFile(filename, zipped_buffer);

                     if (added_element == nullptr)
                        return;
                     added_element->offset_ = off_local_header;

                     // Update file type count
                     added_element->type_ = type_manager_->GetTypeFromBuffer(
                        added_element->Extract(), added_element->size_);
                     if (added_element->type_ == 0)
                        added_element->type_ = type_manager_->GetTypeFromFile(added_element->name_.c_str());
                  }

                  // local file header
                  unsigned char* local_header = &zipped_buffer[off_local_header];
                  if (*(unsigned int*)(&local_header[0]) == 0x04034b50)
                  {
                     short min_version = *(short*)&(local_header[4]);
                     short flags = *(short*)&(local_header[6]);
                     short comp_method = *(short*)&(local_header[8]);

                     if (added_element != nullptr)
                     {
                        added_element->offset_ = off_local_header;
                     }
                  }
                  offset_dir += 46 + filename_size + extra_size + Comment_size;
               }
               else
               {
                  // Error !
                  return;
               }
            }
         }
      }

      delete[]zipped_buffer;
   }
}
#endif

/////////////////////////////////////////////////////////////////
// DirectoryFile Implementation
/////////////////////////////////////////////////////////////////
DirectoryFile::DirectoryFile(ITypeManager* type_manager, const std::string& file): IContainedElement(type_manager)
{
}

std::vector<SingleElements> DirectoryFile::GetInnerElements(int type)
{
   std::vector<SingleElements> list;
   /*SingleElements element;
   element.filename_ = file_name_.c_str();
   element.buffer_ = buffer_;
   element.size_ = buffer_size_;

   list.push_back(element);*/
   return list;
}

/////////////////////////////////////////////////////////////////
// DiskContainer Implementation
/////////////////////////////////////////////////////////////////
DataContainer::DataContainer(ITypeManager* type_manager) : type_manager_(type_manager)
{
   DataContainer::Init();
}

DataContainer::~DataContainer()
{
   Clear();
}

void DataContainer::Clear()
{
   for (std::vector<IContainedElement*>::iterator it = file_list_.begin(); it < file_list_.end(); ++it)
   {
      delete *it;
   }

   file_list_.clear();
}

void DataContainer::Init()
{
}

void DataContainer::AddSourceFile(const char* path)
{
   current_path_ = path;

   //
   BuildFileList();
}

void DataContainer::BuildFileList()
{
   // Is this a file or a directory ?
   if (IsDirectory (current_path_.c_str()))
   {
      BuildFromDirectory();
   }
   else
   {
      // Is this a compressed file ?
      BuildFromFile();
   }
}

void DataContainer::BuildFromDirectory()
{
   // Check directory content.
   IContainedElement* new_element = new DirectoryFile(type_manager_, current_path_);
   file_list_.push_back(new_element);
}

void DataContainer::BuildFromFile()
{
   if (BuildFromZippedFile() == false)
   {
      IContainedElement* new_element = new SingleFile(type_manager_, current_path_);
      file_list_.push_back(new_element);
   }
}

bool DataContainer::BuildFromZippedFile()
{
 
#ifndef NOZLIB
   bool bRet = false;
   // Open zipped content, then check it
   FILE* f;
   if (fopen_s(&f, current_path_.c_str(), "rb") == 0)
   {
      unsigned char zipped_buffer [4];
      fread(zipped_buffer, 4, 1, f);
      fclose(f);

      if (*(unsigned int*)zipped_buffer == 0x04034b50)
      {
         IContainedElement* new_element = new ZippedFile(type_manager_, current_path_);
         file_list_.push_back(new_element);
      }
   }
   return bRet;
#else
   // No handling of zipped file
   return false;
#endif
}


std::vector<IContainedElement*> DataContainer::GetFileList()
{
   std::vector<IContainedElement*> out_list;

   //
   for (std::vector<IContainedElement*>::iterator it = file_list_.begin(); it < file_list_.end(); ++it)
   {
      if ((*it)->GetType() != 0)
      {
         out_list.push_back(*it);
      }
   }

   return out_list;
}


int DataContainer::GetTypeFromDirectory(ITypeManager* manager)
{
   // Check directory content.
   return 0;
}


int DataContainer::GetType(ITypeManager* manager)
{
   auto return_value = 0;

   if (IsDirectory(current_path_.c_str()))
   {
      return_value = GetTypeFromDirectory(manager);
   }
   else
   {
      // Is this a compressed file ?
      return_value = GetTypeFromFile(manager);
   }

   return return_value;
}

#ifndef NOZLIB
int DataContainer::InitUnzip(ITypeManager* manager, unsigned char* zipped_buffer, int size)
{
   auto return_value = -1;

   // ZIP format : Find EOCD
   int offset_eocd = -1;
   for (int offset = size - 1; offset >= 0 && offset_eocd == -1; offset --)
   {
      if (*(unsigned int*)(&zipped_buffer[offset]) == 0x06054B50)
      {
         offset_eocd = offset;
      }
   }
   if (offset_eocd != -1)
   {
      return_value = 0;
      // Found : Get infos.
      int nb_central_dir = *(short*)&(zipped_buffer[offset_eocd + 8]);
      int offset_dir = *(int*)&(zipped_buffer[offset_eocd + 16]);

      // Now, look at central directories
      for (int cd = 0; cd < nb_central_dir; cd ++)
      {
         // Check signature
         if (*(unsigned int*)(&zipped_buffer[offset_dir]) == 0x02014b50)
         {
            // check version min
            // Compression method
            short min_version = *(short*)&(zipped_buffer[offset_dir + 6]);
            short comp_method = *(short*)&(zipped_buffer[offset_dir + 10]);
            short ifile_attrib = *(short*)&(zipped_buffer[offset_dir + 36]);
            int efile_attrib = *(int*)&(zipped_buffer[offset_dir + 38]);

            //
            int off_local_header = *(int*)&(zipped_buffer[offset_dir + 42]);
            // Sizes
            int filename_size = *(short*)(&zipped_buffer[offset_dir + 28]);
            int extra_size = *(short*)(&zipped_buffer[offset_dir + 30]);
            int Comment_size = *(short*)(&zipped_buffer[offset_dir + 32]);
            char* filename = new char [filename_size + 1];
            memcpy(filename, &zipped_buffer[offset_dir + 46], filename_size);
            filename[filename_size] = 0;

            // Directory ?
            NodeFS* added_element;
            if (efile_attrib == FILE_ATTRIBUTE_DIRECTORY)
            {
               // New node :
               added_element = zip_file_.InsertDir(filename);
            }
            else
            {
               // File
               added_element = zip_file_.InsertFile(filename, zipped_buffer);

               added_element->offset_ = off_local_header;
               // Update file type count
               int file_type = manager->GetTypeFromFile(added_element->name_.c_str());
            }

            // local file header
            unsigned char* local_header = &zipped_buffer[off_local_header];
            if (*(unsigned int*)(&local_header[0]) == 0x04034b50)
            {
               short min_version = *(short*)&(local_header[4]);
               short flags = *(short*)&(local_header[6]);
               short comp_method = *(short*)&(local_header[8]);

               if (added_element != nullptr)
               {
                  added_element->offset_ = off_local_header;
               }
            }
            offset_dir += 46 + filename_size + extra_size + Comment_size;
         }
         else
         {
            // Error !
            return -1;
         }
      }
   }
   return return_value;
}

int DataContainer::GetTypeFromZippedFile(ITypeManager* manager)
{
   int ret = 0;
   // Open zipped content, then check it
   FILE* f;
   if (fopen_s(&f, current_path_.c_str(), "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      long size = ftell(f);
      rewind(f);
      unsigned char* zipped_buffer = new unsigned char [size];

      fread(zipped_buffer, size, 1, f);
      fclose(f);

      if (*(unsigned int*)zipped_buffer == 0x04034b50)
      {
         // Init zip file
         ret = InitUnzip(manager, zipped_buffer, size);
      }

      delete []zipped_buffer;
   }

   return ret;
}
#endif


int DataContainer::GetTypeFromFile(ITypeManager* manager)
{
   FILE* f;
#ifndef NOZLIB
   int ret = GetTypeFromZippedFile(manager);
   if (ret != 0)
   {
      return ret;
   }
#else
   int ret = 0;
#endif
   if (fopen_s(&f, current_path_.c_str(), "rb") == 0)
   {
      fseek(f, 0, SEEK_END);
      const long size = ftell(f);
      rewind(f);

      auto* header = new unsigned char[size];

      fread(header, size, 1, f);
      fclose(f);

      // Anything from the buffer ?
      ret = manager->GetTypeFromBuffer(header, size);

      // No, check with name
      if (ret == 0)
      {
         ret = manager->GetTypeFromFile(current_path_.c_str());
      }

      delete []header;
   }

   return ret;
}
