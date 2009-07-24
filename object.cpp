#include "object.h"

#include <cstdio>
#include "data.h"
#include "module.h"
#include "numeric.h"
#include "code.h"

PycRef<PycObject> Pyc_NULL = (PycObject*)0;
PycRef<PycObject> Pyc_None = new PycObject(PycObject::TYPE_NONE);
PycRef<PycObject> Pyc_Ellipsis = new PycObject(PycObject::TYPE_ELLIPSIS);
PycRef<PycObject> Pyc_StopIteration = new PycObject(PycObject::TYPE_STOPITER);
PycRef<PycObject> Pyc_False = new PycObject(PycObject::TYPE_FALSE);
PycRef<PycObject> Pyc_True = new PycObject(PycObject::TYPE_TRUE);

PycRef<PycObject> CreateObject(int type)
{
    switch (type) {
    case PycObject::TYPE_NULL:
        return (PycObject*)0;
    case PycObject::TYPE_NONE:
        return Pyc_None;
    case PycObject::TYPE_FALSE:
        return Pyc_False.cast<PycObject>();
    case PycObject::TYPE_TRUE:
        return Pyc_True.cast<PycObject>();
    case PycObject::TYPE_STOPITER:
        return Pyc_StopIteration;
    case PycObject::TYPE_ELLIPSIS:
        return Pyc_Ellipsis;
    case PycObject::TYPE_INT:
        return new PycInt();
    //case PycObject::TYPE_INT64:
    //    ...
    case PycObject::TYPE_FLOAT:
        return new PycFloat();
    //case PycObject::TYPE_BINARY_FLOAT:
    //    ...
    //case PycObject::TYPE_COMPLEX:
    //    ...
    //case PycObject::TYPE_BINARY_COMPLEX:
    //    ...
    case PycObject::TYPE_LONG:
        return new PycLong();
    case PycObject::TYPE_STRING:
        return new PycString();
    case PycObject::TYPE_INTERNED:
        return new PycString(PycObject::TYPE_INTERNED);
    case PycObject::TYPE_STRINGREF:
        return new PycString(PycObject::TYPE_STRINGREF);
    case PycObject::TYPE_TUPLE:
        return new PycTuple();
    case PycObject::TYPE_LIST:
        return new PycList();
    case PycObject::TYPE_DICT:
        return new PycDict();
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        return new PycCode();
    //case PycObject::TYPE_UNICODE:
    //    ...
    //case PycObject::TYPE_SET:
    //    ...
    //case PycObject::TYPE_FROZENSET:
    //    ...
    default:
        fprintf(stderr, "CreateObject: Got unsupported type 0x%X\n", type);
        return (PycObject*)0;
    }
}

PycRef<PycObject> LoadObject(PycData* stream, PycModule* mod)
{
    PycRef<PycObject> obj = CreateObject(stream->getByte());
    if (obj != Pyc_NULL)
        obj->load(stream, mod);
    return obj;
}
