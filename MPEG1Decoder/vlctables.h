class MacroAddrIncTable : public VLCTable {
private:
    void _buildTable();
public:
    MacroAddrIncTable(BitStream &stream) : VLCTable(stream) {}
};

class MacroTypeITable : public VLCTable {
private:
    void _buildTable();
public:
    MacroTypeITable(BitStream &stream) : VLCTable(stream) {}
};

class MacroTypePTable : public VLCTable {
private:
    void _buildTable();
public:
    MacroTypePTable(BitStream &stream) : VLCTable(stream) {}
};

class MacroTypeBTable : public VLCTable {
private:
    void _buildTable();
public:
    MacroTypeBTable(BitStream &stream) : VLCTable(stream) {}
};

class CodedBlockPatternTable : public VLCTable {
private:
    void _buildTable();
public:
    CodedBlockPatternTable(BitStream &stream) : VLCTable(stream) {}
};

class MotionVectorTable : public VLCTable{
private:
    void _buildTable();
public:
    MotionVectorTable(BitStream &stream) : VLCTable(stream) {}
};

class DCTDCSizeLumaTable : public VLCTable{
private:
    void _buildTable();
public:
    DCTDCSizeLumaTable(BitStream &stream) : VLCTable(stream) {}
};

class DCTDCSizeChromaTable : public VLCTable{
private:
    void _buildTable();
public:
    DCTDCSizeChromaTable(BitStream &stream) : VLCTable(stream) {}
};

class DCTCoeffTable : public VLCTable{
private:
    enum Type { first, next } m_type;
    void _buildTable();
public:
    DCTCoeffTable(BitStream &stream, enum Type type) : VLCTable(stream), m_type(type) {}
};