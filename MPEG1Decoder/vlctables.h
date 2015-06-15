class MacroAddrIncTable : public VLCTable {
protected:
    void _buildTable();
public:
    MacroAddrIncTable(BitStream &stream) : VLCTable(stream) {}
};

class MacroTypeITable : public VLCTable {
protected:
    void _buildTable();
public:
    MacroTypeITable(BitStream &stream) : VLCTable(stream) {}
};

class MacroTypePTable : public VLCTable {
protected:
    void _buildTable();
public:
    MacroTypePTable(BitStream &stream) : VLCTable(stream) {}
};

class MacroTypeBTable : public VLCTable {
protected:
    void _buildTable();
public:
    MacroTypeBTable(BitStream &stream) : VLCTable(stream) {}
};

class CodedBlockPatternTable : public VLCTable {
protected:
    void _buildTable();
public:
    CodedBlockPatternTable(BitStream &stream) : VLCTable(stream) {}
};

class MotionVectorTable : public VLCTable{
protected:
    void _buildTable();
public:
    MotionVectorTable(BitStream &stream) : VLCTable(stream) {}
};

class DCTDCSizeLumaTable : public VLCTable{
protected:
    void _buildTable();
public:
    DCTDCSizeLumaTable(BitStream &stream) : VLCTable(stream) {}
};

class DCTDCSizeChromaTable : public VLCTable{
protected:
    void _buildTable();
public:
    DCTDCSizeChromaTable(BitStream &stream) : VLCTable(stream) {}
};

class DCTCoeffTable : public VLCTable{
protected:
    void _buildTable();
public:
    DCTCoeffTable(BitStream &stream) : VLCTable(stream) {}
};

class DCTCoeffFirstTable : public DCTCoeffTable {
protected:
    void _buildTable();
public:
    DCTCoeffFirstTable(BitStream &stream) : DCTCoeffTable(stream) {}
};

class DCTCoeffNextTable : public DCTCoeffTable {
protected:
    void _buildTable();
public:
    DCTCoeffNextTable(BitStream &stream) : DCTCoeffTable(stream) {}
};