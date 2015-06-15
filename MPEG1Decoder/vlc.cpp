#include "MPEG1Decoder.h"

VLCTable::VLCTable(BitStream &stream) : m_stream(stream) {}

VLCTable::~VLCTable()
{
    if (m_lookup != NULL) delete m_lookup;
}

void VLCTable::init()
{
    _buildTable();
    for (VLCTableEntry &entry : m_table) {
        if (m_len < entry.len) m_len = entry.len;
    }
    m_lookup = new VLCTableEntry*[1 << m_len];
    for (VLCTableEntry &entry : m_table) {
        unsigned start = entry.code << (m_len - entry.len);
        unsigned end = start | (1 << (m_len - entry.len)) - 1;
        for (unsigned i = start; i <= end; i++) m_lookup[i] = &entry;
    }
}

int VLCTable::get()
{
    VLCTableEntry *entry = m_lookup[m_stream.nextbits(m_len, false)];
    m_stream.nextbits(entry->len);
    return entry->value;
}

// tables

void MacroAddrIncTable::_buildTable()
{
}

void MacroTypeITable::_buildTable()
{
}

void MacroTypePTable::_buildTable()
{
}

void MacroTypeBTable::_buildTable()
{
}

void CodedBlockPatternTable::_buildTable()
{
}

void MotionVectorTable::_buildTable()
{
}

void DCTDCSizeLumaTable::_buildTable()
{
}

void DCTDCSizeChromaTable::_buildTable()
{
}

void DCTCoeffTable::_buildTable()
{
}