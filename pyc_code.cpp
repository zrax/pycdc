#include "pyc_code.h"
#include "pyc_module.h"
#include "data.h"

void PycCode::load(PycData* stream, PycModule* mod)
{
    if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
        m_argCount = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_argCount = stream->get32();

    if (mod->majorVer() >= 3)
        m_kwOnlyArgCount = stream->get32();

    if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
        m_numLocals = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_numLocals = stream->get32();

    if (mod->verCompare(1, 5) >= 0 && mod->verCompare(2, 3) < 0)
        m_stackSize = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_stackSize = stream->get32();

    if (mod->verCompare(1, 3) >= 0 && mod->verCompare(2, 3) < 0)
        m_flags = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_flags = stream->get32();

    m_code = LoadObject(stream, mod).cast<PycString>();
    m_consts = LoadObject(stream, mod).cast<PycTuple>();
    m_names = LoadObject(stream, mod).cast<PycTuple>();

    if (mod->verCompare(1, 3) >= 0)
        m_varNames = LoadObject(stream, mod).cast<PycTuple>();

    if (mod->verCompare(2, 1) >= 0)
        m_freeVars = LoadObject(stream, mod).cast<PycTuple>();

    if (mod->verCompare(2, 1) >= 0)
        m_cellVars = LoadObject(stream, mod).cast<PycTuple>();

    m_fileName = LoadObject(stream, mod).cast<PycString>();
    m_name = LoadObject(stream, mod).cast<PycString>();

    if (mod->verCompare(1, 5) >= 0 && mod->verCompare(2, 3) < 0)
        m_firstLine = stream->get16();
    else if (mod->verCompare(2, 3) >= 0)
        m_firstLine = stream->get32();

    if (mod->verCompare(1, 5) >= 0)
        m_lnTable = LoadObject(stream, mod).cast<PycString>();
}
