#include "pyc_object.h"
#include "pyc_module.h"
#include "pyc_numeric.h"
#include "pyc_code.h"
#include "data.h"
#include <cstdio>

PycRef<PycObject> Pyc_None = new PycObject(PycObject::TYPE_NONE);
PycRef<PycObject> Pyc_Ellipsis = new PycObject(PycObject::TYPE_ELLIPSIS);
PycRef<PycObject> Pyc_StopIteration = new PycObject(PycObject::TYPE_STOPITER);
PycRef<PycObject> Pyc_False = new PycObject(PycObject::TYPE_FALSE);
PycRef<PycObject> Pyc_True = new PycObject(PycObject::TYPE_TRUE);

PycRef<PycObject> CreateObject(int type)
{
    switch (type) {
    case PycObject::TYPE_NULL:
        return NULL;
    case PycObject::TYPE_NONE:
        return Pyc_None;
    case PycObject::TYPE_FALSE:
        return Pyc_False;
    case PycObject::TYPE_TRUE:
        return Pyc_True;
    case PycObject::TYPE_STOPITER:
        return Pyc_StopIteration;
    case PycObject::TYPE_ELLIPSIS:
        return Pyc_Ellipsis;
    case PycObject::TYPE_INT:
        return new PycInt(type);
    case PycObject::TYPE_INT64:
        return new PycLong(type);
    case PycObject::TYPE_FLOAT:
        return new PycFloat(type);
    case PycObject::TYPE_BINARY_FLOAT:
        return new PycCFloat(type);
    case PycObject::TYPE_COMPLEX:
        return new PycComplex(type);
    case PycObject::TYPE_BINARY_COMPLEX:
        return new PycCComplex(type);
    case PycObject::TYPE_LONG:
        return new PycLong(type);
    case PycObject::TYPE_STRING:
    case PycObject::TYPE_INTERNED:
    case PycObject::TYPE_STRINGREF:
    case PycObject::TYPE_UNICODE:
    case PycObject::TYPE_ASCII:
    case PycObject::TYPE_ASCII_INTERNED:
    case PycObject::TYPE_SHORT_ASCII:
    case PycObject::TYPE_SHORT_ASCII_INTERNED:
        return new PycString(type);
    case PycObject::TYPE_TUPLE:
    case PycObject::TYPE_SMALL_TUPLE:
        return new PycTuple(type);
    case PycObject::TYPE_LIST:
        return new PycList(type);
    case PycObject::TYPE_DICT:
        return new PycDict(type);
    case PycObject::TYPE_CODE:
    case PycObject::TYPE_CODE2:
        return new PycCode(type);
    case PycObject::TYPE_SET:
    case PycObject::TYPE_FROZENSET:
        return new PycSet(type);
    default:
        fprintf(stderr, "CreateObject: Got unsupported type 0x%X\n", type);
        return NULL;
    }
}

PycRef<PycObject> LoadObject(PycData* stream, PycModule* mod)
{
    int type = stream->getByte();
    PycRef<PycObject> obj;

    if (type == PycObject::TYPE_OBREF) {
        int index = stream->get32();
        obj = mod->getRef(index);
    } else {
        obj = CreateObject(type & 0x7F);
        if (obj != NULL) {
            if (type & 0x80)
                mod->refObject(obj);
            obj->load(stream, mod);
        }
    }

    return obj;
}
