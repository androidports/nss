
/*
 * Supported functions..
 */
#include <pkcs11.h>
#include "nspr.h"
#include "prtypes.h"

typedef enum {
    F_No_Function,
#undef CK_NEED_ARG_LIST
#define CK_PKCS11_FUNCTION_INFO(func) F_##func,
#include "pkcs11f.h"
#undef CK_NEED_ARG_LISt
#undef CK_PKCS11_FUNCTION_INFO
    F_SetVar,
    F_NewArray,
    F_NewTemplate,
    F_NewMechanism,
    F_BuildTemplate,
    F_SetTemplate,
    F_Print,
    F_SaveVar,
    F_RestoreVar,
    F_Delete,
    F_List,
    F_Run,
    F_Load,
    F_Unload,
    F_System,
    F_Quit,
} FunctionType;

/*
 * Supported Argument Types
 */
typedef enum {
    ArgNone,
    ArgVar,
    ArgULong,
    ArgChar,
    ArgUTF8,
    ArgInfo,
    ArgSlotInfo,
    ArgTokenInfo,
    ArgSessionInfo,
    ArgAttribute,
    ArgMechanism,
    ArgMechanismInfo,
    ArgInitializeArgs,
    ArgFunctionList,
/* Modifier Flags */
    ArgMask = 0xff,
    ArgOut = 0x100,
    ArgArray = 0x200,
    ArgNew = 0x400,
    ArgFile = 0x800,
} ArgType;

typedef enum _constType
{
    ConstNone,
    ConstBool,
    ConstInfoFlags,
    ConstSlotFlags,
    ConstTokenFlags,
    ConstSessionFlags,
    ConstMechanismFlags,
    ConstInitializeFlags,
    ConstUsers,
    ConstSessionState,
    ConstObject,
    ConstHardware,
    ConstKeyType,
    ConstCertType,
    ConstAttribute,
    ConstMechanism,
    ConstResult,
    ConstTrust
} ConstType;

typedef struct _constant {
    const char *name;
    CK_ULONG value;
    ConstType type;
    ConstType attrType;
} Constant ;

/*
 * Values structures.
 */
typedef struct _values {
    ArgType	type;
    ConstType	constType;
    int		size;
    char	*filename;
    void	*data;
    int 	reference;
    int		arraySize;
} Value;

/*
 * Variables
 */
typedef struct _variable Variable;
struct _variable {
    Variable *next;
    char *vname;
    Value *value;
};

/* NOTE: if you change MAX_ARGS, you need to change the commands array
 * below as well.
 */

#define MAX_ARGS 10
/*
 * structure for master command array
 */
typedef struct _commands {
    char	*fname;
    FunctionType	fType;
    ArgType	args[MAX_ARGS];
} Commands;

typedef struct _module {
    PRLibrary *library;
    CK_FUNCTION_LIST *functionList;
} Module;


/*
 * the command array itself. Make name to function and it's arguments
 */

extern const char **valueString;
extern const int valueCount;
extern const char **constTypeString;
extern const int constTypeCount;
extern const Constant *consts;
extern const int constCount;
extern const Commands *commands;
extern const int commandCount;

