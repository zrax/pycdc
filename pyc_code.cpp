#include "pyc_code.h"
#include "pyc_module.h"
#include "data.h"

/* == Marshal structure for Code object ==
                1.0     1.3     1.5     2.1     2.3     3.0     3.8     3.11
argcount                short   short   short   long    long    long    long
posonlyargc                                                     long    long
kwonlyargc                                              long    long    long
nlocals                 short   short   short   long    long    long
stacksize                       short   short   long    long    long    long
flags                   short   short   short   long    long    long    long
code            Obj     Obj     Obj     Obj     Obj     Obj     Obj     Obj
consts          Obj     Obj     Obj     Obj     Obj     Obj     Obj     Obj
names           Obj     Obj     Obj     Obj     Obj     Obj     Obj     Obj
varnames                Obj     Obj     Obj     Obj     Obj     Obj
freevars                                Obj     Obj     Obj     Obj
cellvars                                Obj     Obj     Obj     Obj
locals+names                                                            Obj
locals+kinds                                                            Obj
filename        Obj     Obj     Obj     Obj     Obj     Obj     Obj     Obj
name            Obj     Obj     Obj     Obj     Obj     Obj     Obj     Obj
qualname                                                                Obj
firstline                       short   short   long    long    long    long
lntable                         Obj     Obj     Obj     Obj     Obj     Obj
exceptiontable                                                          Obj
*/

void PycCode::load(PycData* stream, PycModule* mod)
{
    if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
        m_argCount = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_argCount = stream->get32();

    if (mod->verCompare(3, 8) >= 0)
        m_posOnlyArgCount = stream->get32();
    else
        m_posOnlyArgCount = 0;

    if (mod->majorVer() >= 3)
        m_kwOnlyArgCount = stream->get32();
    else
        m_kwOnlyArgCount = 0;

    if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
        m_numLocals = stream->get16();
    else if (mod->verCompare(2, 3) >= 0 && mod->verCompare(3, 11) < 0)
        m_numLocals = stream->get32();
    else
        m_numLocals = 0;

    if (mod->verCompare(1, 5) >= 0 && mod->verCompare(2, 3) < 0)
        m_stackSize = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_stackSize = stream->get32();
    else
        m_stackSize = 0;

    if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
        m_flags = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_flags = stream->get32();
    else
        m_flags = 0;

    if (mod->verCompare(3, 8) < 0) {
        // Remap flags to new values introduced in 3.8
        if (m_flags & 0xF0000000)
            throw std::runtime_error("Cannot remap unexpected flags");
        m_flags = (m_flags & 0xFFFF) | ((m_flags & 0xFFF0000) << 4);
    }

    m_code = LoadObject(stream, mod).cast<PycString>();
    m_consts = LoadObject(stream, mod).cast<PycSequence>();
    m_names = LoadObject(stream, mod).cast<PycSequence>();

    // Represents localsplusnames for py 3.11+
    if (mod->verCompare(1, 3) >= 0)
        m_localNames = LoadObject(stream, mod).cast<PycSequence>();
    else
        m_localNames = new PycTuple;

    PycSimpleSequence::value_t free_vars_extra, cell_vars_extra;

    if (mod->verCompare(3, 11) >= 0) {
        m_localKinds = LoadObject(stream, mod).cast<PycString>();

        if (m_localKinds->length() != m_localNames->size()) {
            throw std::runtime_error("All variables kinds are not available");
        }

        // Starting py3.11, all variables are now part of localsplusnames

        for (int i = 0; i < m_localKinds->length(); i++) {
            const char kind = m_localKinds->value()[i];
            auto name = m_localNames->get(i);

            if (kind & Kinds::CO_FAST_CELL) {
                cell_vars_extra.push_back(name);
            }
            else if (kind & Kinds::CO_FAST_FREE) {
                free_vars_extra.push_back(name);
            }
        }
    }
    else
        m_localKinds = new PycString;

    if (mod->verCompare(2, 1) >= 0 && mod->verCompare(3, 11) < 0)
        m_freeVars = LoadObject(stream, mod).cast<PycSequence>();
    else if (mod->verCompare(3, 11) >= 0)
        m_freeVars = new PycTuple(free_vars_extra);
    else
        m_freeVars = new PycTuple;

    if (mod->verCompare(2, 1) >= 0 && mod->verCompare(3, 11) < 0)
        m_cellVars = LoadObject(stream, mod).cast<PycSequence>();
    else if (mod->verCompare(3, 11) >= 0)
        m_cellVars = new PycTuple(cell_vars_extra);
    else
        m_cellVars = new PycTuple;

    m_fileName = LoadObject(stream, mod).cast<PycString>();
    m_name = LoadObject(stream, mod).cast<PycString>();

    if (mod->verCompare(3, 11) >= 0)
        m_qualName = LoadObject(stream, mod).cast<PycString>();
    else
        m_qualName = new PycString;

    if (mod->verCompare(1, 5) >= 0 && mod->verCompare(2, 3) < 0)
        m_firstLine = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_firstLine = stream->get32();

    if (mod->verCompare(1, 5) >= 0)
        m_lnTable = LoadObject(stream, mod).cast<PycString>();
    else
        m_lnTable = new PycString;

    if (mod->verCompare(3, 11) >= 0)
        m_exceptTable = LoadObject(stream, mod).cast<PycString>();
    else
        m_exceptTable = new PycString;
}

PycRef<PycString> PycCode::getCellVar(PycModule* mod, int idx) const
{
    if (mod->verCompare(3, 11) >= 0)
        return getLocal(idx);

    return (idx >= m_cellVars->size())
        ? m_freeVars->get(idx - m_cellVars->size()).cast<PycString>()
        : m_cellVars->get(idx).cast<PycString>();
}

int _parse_varint(PycBuffer& data, int& pos) {
    int b = data.getByte();
    pos += 1;

    int val = b & 0x3F;
    while (b & 0x40) {
        val <<= 6;

        b = data.getByte();
        pos += 1;

        val |= (b & 0x3F);
    }
    return val;
}

std::vector<PycExceptionTableEntry> PycCode::exceptionTableEntries() const
{
    PycBuffer data(m_exceptTable->value(), m_exceptTable->length());

    std::vector<PycExceptionTableEntry> entries;

    int pos = 0;
    while (!data.atEof()) {

        int start = _parse_varint(data, pos) * 2;
        int length = _parse_varint(data, pos) * 2;
        int end = start + length;
        
        int target = _parse_varint(data, pos) * 2;
        int dl = _parse_varint(data, pos);

        int depth = dl >> 1;
        bool lasti = bool(dl & 1);
        
        entries.push_back(PycExceptionTableEntry(start, end, target, depth, lasti));
    }
    
    return entries;
}
