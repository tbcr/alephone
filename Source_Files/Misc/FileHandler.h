#ifndef _FILE_HANDLER_
#define _FILE_HANDLER_
/*
	File-handler classes
	by Loren Petrich,
	August 11, 2000

	These are designed to provide some abstract interfaces to file and directory objects.
	
	Most of these routines return whether they had succeeded;
	more detailed error codes are API-specific.
	Attempted to support stdio I/O directly, but on my Macintosh, at least,
	the performance was much poorer. This is possibly due to "fseek" having to
	actually read the file or something.
	
	Merged all the Macintosh-specific code into these base classes, so that
	it will be selected with a preprocessor statement when more than one file-I/O
	API is supported.
*/


// For the filetypes
#include "tags.h"

#include <stddef.h>	// For size_t
#include <time.h>	// For time_t

#ifdef SDL
#include <errno.h>
#include <string>
#define fnfErr ENOENT
#endif


// Symbolic constant for a closed file's reference number (refnum) (MacOS only)
const short RefNum_Closed = -1;


/*
	Abstraction for opened files; it does reading, writing, and closing of such files,
	without doing anything to the files' specifications
*/
class OpenedFile
{
	// This class will need to set the refnum and error value appropriately 
	friend class FileSpecifier;
	
public:
	bool IsOpen();
	bool Close();
	
	bool GetPosition(long& Position);
	bool SetPosition(long Position);
	
	bool GetLength(long& Length);
	bool SetLength(long Length);
	
	bool Read(long Count, void *Buffer);
	bool Write(long Count, void *Buffer);
	
	// Smart macros for more easy reading of structured objects
	// CB: reading/writing C structures from/to files is evil...
	
	template<class T> bool ReadObject(T& Object)
		{return Read(sizeof(T),&Object);}
	
	template<class T> bool ReadObjectList(int NumObjects, T* ObjectList)
		{return Read(NumObjects*sizeof(T),ObjectList);}
	
	template<class T> bool WriteObject(T& Object)
		{return Write(sizeof(T),&Object);}
	
	template<class T> bool WriteObjectList(int NumObjects, T* ObjectList)
		{return Write(NumObjects*sizeof(T),ObjectList);}
	
	OpenedFile();
	~OpenedFile() {Close();}	// Auto-close when destroying

	// Platform-specific members
#if defined(mac)

	short GetRefNum() {return RefNum;}
	OSErr GetError() {return Err;}

private:
	short RefNum;	// File reference number
	OSErr Err;		// Error code

#elif defined(SDL)

	int GetError() {return errno;}
	SDL_RWops *GetRWops() {return f;}

private:
	SDL_RWops *f;	// File handle

#endif
};


/*
	Abstraction for loaded resources;
	this object will release that resource when it finishes.
	MacOS resource handles will be assumed to be locked.
*/
class LoadedResource
{
	// This class grabs a resource to be loaded into here
	friend class OpenedResourceFile;
	
public:
	// Resource loaded?
	bool IsLoaded();
	
	// Unloads the resource
	void Unload();
	
	// Get size of loaded resource
	size_t GetLength();
	
	// Get pointer (always present)
	void *GetPointer(bool DoDetach = false);
	
	LoadedResource();
	~LoadedResource() {Unload();}	// Auto-unload when destroying

private:
	// Detaches an allocated resource from this object
	// (keep private to avoid memory leaks)
	void Detach();

public:
	// Platform-specific members
#if defined(mac)

	Handle GetHandle(bool DoDetach = false)
		{Handle ret = RsrcHandle; if (DoDetach) Detach(); return ret;}
	
	// Gets some suitable default color table
	void GetSystemColorTable()
		{Unload(); RsrcHandle = Handle(GetCTable(8));}

private:
	Handle RsrcHandle;
	
#elif defined(SDL)

	void *p;
	uint32 size;
#endif
};


/*
	Abstraction for opened resource files:
	it does opening, setting, and closing of such files;
	also getting "LoadedResource" objects that return pointers
*/
class OpenedResourceFile
{
	// This class will need to set the refnum and error value appropriately 
	friend class FileSpecifier;
	
public:
	
	// Pushing and popping the current file -- necessary in the MacOS version,
	// since resource forks are globally open with one of them the current top one.
	// Push() saves the earlier top one makes the current one the top one,
	// while Pop() restores the earlier top one.
	// Will leave SetResLoad in the state of true.
	bool Push();
	bool Pop();

	// Pushing and popping are unnecessary for the MacOS versions of Get() and Check()
	// Check simply checks if a resource is present; returns whether it is or not
	// Get loads a resource; returns whether or not one had been successfully loaded
	// CB: added functions that take 4 characters instead of uint32, which is more portable
	bool Check(uint32 Type, int16 ID);
	bool Check(uint8 t1, uint8 t2, uint8 t3, uint8 t4, int16 ID) {return Check(FOUR_CHARS_TO_INT(t1, t2, t3, t4), ID);}
	bool Get(uint32 Type, int16 ID, LoadedResource& Rsrc);
	bool Get(uint8 t1, uint8 t2, uint8 t3, uint8 t4, int16 ID, LoadedResource& Rsrc) {return Get(FOUR_CHARS_TO_INT(t1, t2, t3, t4), ID, Rsrc);}

	bool IsOpen();
	bool Close();
	
	OpenedResourceFile();
	~OpenedResourceFile() {Close();}	// Auto-close when destroying

	// Platform-specific members
#if defined(mac)

	short GetRefNum() {return RefNum;}
	OSErr GetError() {return Err;}

private:
	short RefNum;	// File reference number
	OSErr Err;		// Error code
	
	short SavedRefNum;

#elif defined(SDL)

	int GetError() {return errno;}

private:
	SDL_RWops *f, *saved_f;
#endif
};


#ifdef mac
/*
	Abstraction for directory specifications;
	designed to encapsulate both directly-specified paths
	and MacOS volume/directory ID's.
*/
class DirectorySpecifier
{
	// This class needs to see directory info 
	friend class FileSpecifier;
	
	// MacOS directory specification
	long parID;
	short vRefNum;
	
	OSErr Err;
public:

	short Get_vRefNum() {return vRefNum;}
	long Get_parID() {return parID;}
	void Set_vRefNum(short _vRefNum) {vRefNum = _vRefNum;}
	void Set_parID(long _parID) {parID = _parID;}
	
	// Set special directories:
	bool SetToAppParent();
	bool SetToPreferencesParent();
	
	DirectorySpecifier& operator=(DirectorySpecifier& D)
		{vRefNum = D.vRefNum; parID = D.parID; return *this;}
		
	OSErr GetError() {return Err;}
	
	DirectorySpecifier(): vRefNum(0), parID(0) {}
	DirectorySpecifier(DirectorySpecifier& D) {*this = D;}
};
#else
#define DirectorySpecifier FileSpecifier
#endif


// Time-specification data type (can be set to 64-bit if desired)
#if defined(mac)
typedef unsigned long TimeType;
#else
typedef time_t TimeType;	// Maybe this can also be used on MacOS, so there wouldn't be any need for TimeType?
#endif


/*
	Abstraction for file specifications;
	designed to encapsulate both directly-specified paths
	and MacOS FSSpecs
*/
class FileSpecifier
{	
public:
	// The typecodes here are the symbolic constants defined in tags.h (_typecode_creator, etc.)
	
	// The name as a C string:
	// assumes enough space to hold it if getting (max. 256 bytes)
	// The typecode is for automatically adding a suffix;
	// NONE means add none
	
	void GetName(char *Name);
	void SetName(char *Name, int Type);
	
#ifdef mac
	// Move the directory specification
	void ToDirectory(DirectorySpecifier& Dir);
	void FromDirectory(DirectorySpecifier& Dir);
#endif
	
	// Partially inspired by portable_files.h:
	
	// These functions take an appropriate one of the typecodes used earlier;
	// this is to try to cover the cases of both typecode attributes
	// and typecode suffixes.
	bool Create(int Type);
	
	// Opens a file:
	bool Open(OpenedFile& OFile, bool Writable=false);
	
	// Opens either a MacOS resource fork or some imitation of it:
	bool Open(OpenedResourceFile& OFile, bool Writable=false);
	
	// These calls are for creating dialog boxes to set the filespec
	// A null pointer means an empty string
	bool ReadDialog(int Type, char *Prompt=NULL);
	bool WriteDialog(int Type, char *Prompt=NULL, char *DefaultName=NULL);
	
	// Write dialog box for savegames (must be asynchronous, allowing the sound
	// to continue in the background)
	bool WriteDialogAsync(int Type, char *Prompt=NULL, char *DefaultName=NULL);
	
	// Check on whether a file exists, and its type
	bool Exists();
	
	// Gets the modification date
	TimeType GetDate();
	
	// Returns NONE if the type could not be identified;
	// the types returned are the _typecode_stuff in tags.h
	int GetType();
	
	// How many bytes are free in the disk that the file lives in?
	bool GetFreeSpace(unsigned long& FreeSpace);
	
	// Copy file contents
	bool CopyContents(FileSpecifier& File);
	
	// Delete file
	bool Delete();

	// Copy file specification
	const FileSpecifier &operator=(const FileSpecifier &other);
	
	// Platform-dependent parts
#ifdef mac

	FileSpecifier(): Err(noErr) {}
	FileSpecifier(FileSpecifier& F) {*this = F;}
	bool operator==(FileSpecifier& F);
	bool operator!=(FileSpecifier& F) {return !(*this == F);}

	// Filespec management
	void SetSpec(FSSpec& _Spec);
	FSSpec& GetSpec() {return Spec;}
	
	// Set special directories:
	bool SetToApp();
	bool SetParentToPreferences();

	// The error:
	OSErr GetError() {return Err;}
	
private:
	FSSpec Spec;
	OSErr Err;

#elif defined(SDL)

	FileSpecifier() {}
	FileSpecifier(const string &s) : name(s) {}
	FileSpecifier(const char *s) : name(s) {}
	FileSpecifier(const FileSpecifier &other) : name(other.name) {}

	void SetToLocalDataDir();	// Per-user directory, where prefs, recordings and saved games go
	void SetToGlobalDataDir();	// Data file directory, where "Images", "Shapes" etc. are stored

	void AddPart(const string &part);
	void GetLastPart(char *part);
	const char *GetName(void) {return name.c_str();}

	int GetError() {return errno;}

private:
	string name;	// Path name

#endif
};

#endif
