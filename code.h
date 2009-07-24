#ifndef _PYC_CODE_H
#define _PYC_CODE_H

#include "sequence.h"
#include "string.h"

class PycCode : public PycObject {
public:
    PycCode(int type = TYPE_CODE)
        : PycObject(type), m_argCount(0), m_kwOnlyArgCount(0), m_numLocals(0),
          m_stackSize(0), m_flags(0), m_firstLine(0) { }

    bool isType(int type) const
    { return (type == TYPE_CODE) || (type == TYPE_CODE2) || PycObject::isType(type); }

    void load(class PycData* stream, class PycModule* mod);

    int argCount() const { return m_argCount; }
    int kwOnlyArgCount() const { return m_kwOnlyArgCount; }
    int numLocals() const { return m_numLocals; }
    int stackSize() const { return m_stackSize; }
    int flags() const { return m_flags; }
    PycRef<PycString> code() const { return m_code; }
    PycRef<PycTuple> consts() const { return m_consts; }
    PycRef<PycTuple> names() const { return m_names; }
    PycRef<PycTuple> varNames() const { return m_varNames; }
    PycRef<PycTuple> freeVars() const { return m_freeVars; }
    PycRef<PycTuple> cellVars() const { return m_cellVars; }
    PycRef<PycString> fileName() const { return m_fileName; }
    PycRef<PycString> name() const { return m_name; }
    int firstLine() const { return m_firstLine; }
    PycRef<PycString> lnTable() const { return m_lnTable; }

    PycRef<PycObject> getConst(int idx) const
    { return m_consts->values()[idx]; }

    PycRef<PycString> getName(int idx) const
    { return m_names->values()[idx].cast<PycString>(); }

    PycRef<PycString> getVarName(int idx) const
    { return m_varNames->values()[idx].cast<PycString>(); }

    PycRef<PycObject> getCellVar(int idx) const
    {
        return (idx > m_cellVars->size()) ? m_freeVars->values()[idx - m_cellVars->size()]
                                          : m_cellVars->values()[idx];
    }

private:
    int m_argCount, m_kwOnlyArgCount, m_numLocals, m_stackSize, m_flags;
    PycRef<PycString> m_code;
    PycRef<PycTuple> m_consts;
    PycRef<PycTuple> m_names;
    PycRef<PycTuple> m_varNames;
    PycRef<PycTuple> m_freeVars;
    PycRef<PycTuple> m_cellVars;
    PycRef<PycString> m_fileName;
    PycRef<PycString> m_name;
    int m_firstLine;
    PycRef<PycString> m_lnTable;
};

#endif
