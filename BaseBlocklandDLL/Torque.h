#pragma once
#include <windows.h>
#include <map>
#include "include\v8.h"
#include "uv8.h"

using namespace v8;

typedef unsigned int U32;
typedef signed int S32;
typedef float F32;
typedef double F64;
typedef U32 SimObjectId;

struct SimObject;

typedef const char *(*StringCallback)(SimObject *obj, int argc, const char* argv[]);
typedef int(*IntCallback)   (SimObject *obj, int argc, const char* argv[]);
typedef float(*FloatCallback) (SimObject *obj, int argc, const char* argv[]);
typedef void(*VoidCallback)  (SimObject *obj, int argc, const char* argv[]);
typedef bool(*BoolCallback)  (SimObject *obj, int argc, const char* argv[]);


struct Identifier {
	const char* mNamespace;
	const char* mName;
};

struct Namespace
{
	const char* mName;
	const char* mPackage;

	Namespace *mParent;
	Namespace *mNext;
	void *mClassRep;
	U32 mRefCountToParent;
	struct Entry
	{
		enum
		{
			GroupMarker = -3,
			OverloadMarker = -2,
			InvalidFunctionType = -1,
			ScriptFunctionType,
			StringCallbackType,
			IntCallbackType,
			FloatCallbackType,
			VoidCallbackType,
			BoolCallbackType
		};

		Namespace *mNamespace;
		//char _padding1[4];
		Entry *mNext;
		const char *mFunctionName;
		S32 mType;
		S32 mMinArgs;
		S32 mMaxArgs;
		const char *mUsage;
		const char *mPackage;
		void *mCode; // CodeBlock *mCode;
		U32 mFunctionOffset;
		union {
			StringCallback mStringCallbackFunc;
			IntCallback mIntCallbackFunc;
			VoidCallback mVoidCallbackFunc;
			FloatCallback mFloatCallbackFunc;
			BoolCallback mBoolCallbackFunc;
			const char* mGroupName;
		} cb;
	};
	Entry *mEntryList;
	Entry **mHashTable;
	U32 mHashSize;
	U32 mHashSequence;  ///< @note The hash sequence is used by the autodoc console facility
						///        as a means of testing reference state.
	char * lastUsage;
};

struct ConsoleObject
{
};

enum ACRFieldTypes
{
	StartGroupFieldType = 0xFFFFFFFD,
	EndGroupFieldType = 0xFFFFFFFE,
	DepricatedFieldType = 0xFFFFFFFF
};

S32 HashPointer(const char* ptr);

class SimFieldDictionary
{
	friend class SimFieldDictionaryIterator;

public:
	struct Entry
	{
		const char* slotName;
		char *value;
		Entry *next;
	};
private:
	enum
	{
		HashTableSize = 19
	};
	Entry *mHashTable[HashTableSize];

	static Entry *mFreeList;
	static void freeEntry(Entry *entry);
	static Entry *allocEntry();

	/// In order to efficiently detect when a dynamic field has been
	/// added or deleted, we increment this every time we add or
	/// remove a field.
	U32 mVersion;

public:
	const U32 getVersion() const { return mVersion; }

	SimFieldDictionary();
	~SimFieldDictionary();
	void setFieldValue(const char* slotName, const char *value);
	const char *getFieldValue(const char* slotName);
	//void writeFields(SimObject *obj, Stream &strem, U32 tabStop);
	void printFields(SimObject *obj);
	void assignFrom(SimFieldDictionary *dict);
};

class SimFieldDictionaryIterator
{
	SimFieldDictionary *          mDictionary;
	S32                           mHashIndex;
	SimFieldDictionary::Entry *   mEntry;

public:
	SimFieldDictionaryIterator(SimFieldDictionary*);
	SimFieldDictionary::Entry* operator++();
	SimFieldDictionary::Entry* operator*();
};

struct SimObject
{
	enum {
#define BIT(x) (1 << (x))
		Deleted = BIT(0),   ///< This object is marked for deletion.
		Removed = BIT(1),   ///< This object has been unregistered from the object system.
		Added = BIT(3),   ///< This object has been registered with the object system.
		Selected = BIT(4),   ///< This object has been marked as selected. (in editor)
		Expanded = BIT(5),   ///< This object has been marked as expanded. (in editor)
							 //ModStaticFields = BIT(6),    ///< The object allows you to read/modify static fields
							 //ModDynamicFields = BIT(7)     ///< The object allows you to read/modify dynamic fields
							 // why are these different?
							 ModStaticFields = BIT(7),    ///< The object allows you to read/modify static fields
							 ModDynamicFields = BIT(8)     ///< The object allows you to read/modify dynamic fields
	};

	char _padding1[4];
	const char *objectName;
	// char _padding2[24];
	// SimObjectId mId;
	SimObject *nextNameObject;
	SimObject *nextManagerNameObject;
	SimObject *nextIdObject;
	void *mGroup;
	unsigned int mFlags;
	void *mNotifyList;
	SimObjectId id;
	Namespace *mNameSpace;
	unsigned int mTypeMask;
	void *mFieldDictionary;
	void setDatablock(SimObject* obj, const char* datablock);
};

int SimSet__getCount(DWORD set);
SimObject* SimSet__getObject(DWORD set, int index);
bool trySimObjectRef(SimObject** check);
extern int verbosity;
extern v8::Isolate* _Isolate;
extern Persistent<Context> _Context;
Local<Context> ContextL();
void tls_wrapper_init(Isolate* this_, Handle<ObjectTemplate> global);

static const char* ts_js_bridge(SimObject* this_, int argc, const char* argv[]);
void ts_bridge_init(Isolate* this_, Local<ObjectTemplate> global);
void sqlite_driver_init(Isolate* this_, Local<ObjectTemplate> global);

struct SimDatablock: public SimObject {
	typedef SimObject Parent;
	S32 modifiedKey;
	SimObjectId sNextObjectId;
	S32 sNextModifiedKey;
};

struct sqlite_cb_js {
	Isolate* this_;
	Persistent<Function> cbfn;
	Persistent<Object> objref;
};

struct BitSet32 {
	U32 mbits;
};

struct EnumTable
{
	/// Number of enumerated items in the table.
	S32 size;

	/// This represents a specific item in the enumeration.
	struct Enums
	{
		S32 index;        ///< Index label maps to.
		const char *label;///< Label for this index.
	};

	Enums *table;

	/// Constructor.
	///
	/// This sets up the EnumTable with predefined data.
	///
	/// @param sSize  Size of the table.
	/// @param sTable Pointer to table of Enums.
	///
	/// @see gLiquidTypeTable
	/// @see gAlignTable
	EnumTable(S32 sSize, Enums *sTable)
	{
		size = sSize; table = sTable;
	}
};

struct Field {
	const char* pFieldname;    ///< Name of the field.
	const char* pGroupname;      ///< Optionally filled field containing the group name.
								 ///
								 ///  This is filled when type is StartField or EndField

	const char*    pFieldDocs;    ///< Documentation about this field; see consoleDoc.cc.
	bool           groupExpand;   ///< rip expanded/not state of this group in the editor.
	U32            type;          ///< A type ID. @see ACRFieldTypes
	U32            offset;        ///< Memory offset from beginning of class for this field.
	S32            elementCount;  ///< Number of elements, if this is an array.
	EnumTable*    table;         ///< If this is an enum, this points to the table defining it.
	BitSet32       flag;          ///< Stores various flags
	void *validator;     ///< Validator, if any.
	//void * setDataFn;     ///< Set data notify Fn
	//void*  getDataFn;     ///< Get data notify Fn
};

struct SimEvent
{
	SimEvent *nextEvent;
	unsigned int startTime;
	unsigned int time;
	unsigned int sequenceCount;
	SimObject *destObject;
};


//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Macros

//Typedef an engine function to use it later
#define BLFUNC(returnType, convention, name, ...)         \
	typedef returnType (convention*name##Fn)(__VA_ARGS__); \
	static name##Fn name;

//Typedef an exported engine function to use it later
#define BLFUNC_EXTERN(returnType, convention, name, ...)  \
	typedef returnType (convention*name##Fn)(__VA_ARGS__); \
	extern name##Fn name;

//Search for an engine function in blockland
#define BLSCAN(target, pattern, mask)            \
	target = (target##Fn)ScanFunc(pattern, mask); \
	if(target == NULL)                             \
		Printf("torquedll | Cannot find function "#target"!");

//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
//Engine function declarations

Namespace::Entry* fastLookup(const char* ourNamespace, const char* name);
//Con::printf
const char* ToCString(const v8::String::Utf8Value& value);
void* ts__fastCall(Namespace::Entry* ourCall, SimObject* obj, unsigned argc, ...);
BLFUNC_EXTERN(void, , Printf, const char* format, ...);

extern const char *StringTableEntry(const char *str, bool caseSensitive = false);
extern DWORD StringTable;
extern bool setDatablock;
BLFUNC_EXTERN(U32, , Con__tabComplete, char* inputBuffer, U32 cursorPos, U32 maxResultLength, bool forwardTab);
BLFUNC_EXTERN(bool, , initGame, int argc, const char **argv);
BLFUNC_EXTERN(Namespace *, , LookupNamespace, const char *ns);
BLFUNC_EXTERN(const char *, __thiscall, StringTableInsert, DWORD stringTablePtr, const char* val, const bool caseSensitive)
BLFUNC_EXTERN(Namespace::Entry *, __thiscall, Namespace__lookup, Namespace *this_, const char *name)
//BLFUNC_EXTERN(void *, __thiscall, CodeBlock__exec, void *this_, U32 offset, const char *fnName, Namespace *ns, U32 argc, const char **argv, bool noCalls, const char *packageName, int setFrame)
BLFUNC_EXTERN(const char *, __thiscall, CodeBlock__exec, void *this_, U32 offset, Namespace *ns, const char *fnName, U32 argc, const char **argv, bool noCalls, const char *packageName, int setFrame)
BLFUNC_EXTERN(SimObject *, , Sim__findObject_name, const char *name);
BLFUNC_EXTERN(SimObject *, , Sim__findObject_id, unsigned int id);
BLFUNC_EXTERN(void, __thiscall, Container__cleanupSearchVectors, void* this_);
BLFUNC_EXTERN(unsigned int, , Sim__postEvent, SimObject *destObject, SimEvent *event, U32 time);
BLFUNC_EXTERN(void, , Sim__cancelEvent, unsigned int eventSequence);
BLFUNC_EXTERN(void, __thiscall, SimObject__setDataField, SimObject *this_, const char *name, const char *arr, const char *value)
BLFUNC_EXTERN(const char *, __thiscall, SimObject__getDataField, SimObject *this_, const char *name, const char *arr);
BLFUNC_EXTERN(bool, __thiscall, SimObject__registerObject, SimObject *this_);
BLFUNC_EXTERN(void, __thiscall, SimObject__registerReference, SimObject *this_, SimObject **ptr);
BLFUNC_EXTERN(void, __thiscall, SimObject__unregisterReference, SimObject *this_, SimObject **ptr);
BLFUNC_EXTERN(ConsoleObject *, , AbstractClassRep_create_className, const char *className);
/*
BLFUNC_EXTERN(void, , SimObject__setDataBlock, SimObject *this_, const char *datablock);
BLFUNC_EXTERN(bool, , fxDTSBrick__plant, SimObject *this_);
BLFUNC_EXTERN(void, , fxDTSBrick__setTrusted, SimObject *this_, const char* kappa);
*/
BLFUNC_EXTERN(void, __thiscall, SimObject__delete, SimObject *this_);
//This function is really ..odd.
//Help.
//new metario to the rescue!! fuck that!! we're making the user do it on his/her/idk about your pronouns own!!
//BLFUNC_EXTERN(void, __thiscall, fxDTSBrick__setDataBlock, SimObject *this_, const char *dataBlock);
DWORD ScanFunc(const char* pattern, const char* mask);
void PatchByte(BYTE* location, BYTE value);
Namespace::Entry* passThroughLookup(Namespace* ns, const char* name);
void ConsoleFunction(const char* scope, const char* name, StringCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* scope, const char* name, IntCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* scope, const char* name, FloatCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* scope, const char* name, VoidCallback callBack, const char* usage, int minArgs, int maxArgs);
void ConsoleFunction(const char* scope, const char* name, BoolCallback callBack, const char* usage, int minArgs, int maxArgs);

void ConsoleVariable(const char *name, int *data);
void ConsoleVariable(const char *name, bool *data);
void ConsoleVariable(const char *name, float *data);
void ConsoleVariable(const char *name, char *data);

//Evaluate a torquescript string in global scope
const char* Eval(const char* str);

//Call a function
BLFUNC_EXTERN(void, , RawCall, S32 argc, const char* argv);

BLFUNC_EXTERN(void, , SetGlobalVariable, const char *name, const char *value);
BLFUNC_EXTERN(char *, , GetGlobalVariable, const char *name);

typedef void(__thiscall *ShapeNameHudOnRenderFn)(DWORD* obj, DWORD arg1, DWORD arg2, DWORD arg3);
// typedef void(__fastcall *ShapeNameHudOnRenderFn)(SimObject *this_, int, int arg1, int arg2);
static ShapeNameHudOnRenderFn ShapeNameHudOnRender;

bool torque_init();
