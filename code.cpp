#include "code.h"
#include "data.h"
#include "module.h"

void PycCode::load(PycData* stream, PycModule* mod)
{
    if ((mod->majorVer() == 1 && mod->minorVer() >= 3) ||
        (mod->majorVer() == 2 && mod->minorVer() < 3)) {
        m_argCount = stream->get16();
    } else if ((mod->majorVer() == 2 && mod->minorVer() >= 3) ||
               (mod->majorVer() >= 3)) {
        m_argCount = stream->get32();
    }

    if (mod->majorVer() >= 3) {
        m_kwOnlyArgCount = stream->get32();
    }

    if ((mod->majorVer() == 1 && mod->minorVer() >= 3) ||
        (mod->majorVer() == 2 && mod->minorVer() < 3)) {
        m_numLocals = stream->get16();
    } else if ((mod->majorVer() == 2 && mod->minorVer() >= 3) ||
               (mod->majorVer() >= 3)) {
        m_numLocals = stream->get32();
    }

    if ((mod->majorVer() == 1 && mod->minorVer() >= 5) ||
        (mod->majorVer() == 2 && mod->minorVer() < 3)) {
        m_stackSize = stream->get16();
    } else if ((mod->majorVer() == 2 && mod->minorVer() >= 3) ||
               (mod->majorVer() >= 3)) {
        m_stackSize = stream->get32();
    }

    if ((mod->majorVer() == 1 && mod->minorVer() >= 3) ||
        (mod->majorVer() == 2 && mod->minorVer() < 3)) {
        m_flags = stream->get16();
    } else if ((mod->majorVer() == 2 && mod->minorVer() >= 3) ||
               (mod->majorVer() >= 3)) {
        m_flags = stream->get32();
    }

    m_code = LoadObject(stream, mod).cast<PycString>();
    m_consts = LoadObject(stream, mod).cast<PycTuple>();
    m_names = LoadObject(stream, mod).cast<PycTuple>();

    if ((mod->majorVer() == 1 && mod->minorVer() >= 3) ||
        (mod->majorVer() >= 2)) {
        m_varNames = LoadObject(stream, mod).cast<PycTuple>();
    }

    if ((mod->majorVer() == 2 && mod->minorVer() >= 1) ||
        (mod->majorVer() >= 3)) {
        m_freeVars = LoadObject(stream, mod).cast<PycTuple>();
    }

    if ((mod->majorVer() == 2 && mod->minorVer() >= 1) ||
        (mod->majorVer() >= 3)) {
        m_cellVars = LoadObject(stream, mod).cast<PycTuple>();
    }

    m_fileName = LoadObject(stream, mod).cast<PycString>();
    m_name = LoadObject(stream, mod).cast<PycString>();

    if ((mod->majorVer() == 1 && mod->minorVer() >= 5) ||
        (mod->majorVer() == 2 && mod->minorVer() < 3)) {
        m_firstLine = stream->get16();
    } else if ((mod->majorVer() == 2 && mod->minorVer() >= 3) ||
               (mod->majorVer() >= 3)) {
        m_firstLine = stream->get32();
    }

    if ((mod->majorVer() == 1 && mod->minorVer() >= 5) ||
        (mod->majorVer() >= 2)) {
        m_lnTable = LoadObject(stream, mod).cast<PycString>();
    }
}
