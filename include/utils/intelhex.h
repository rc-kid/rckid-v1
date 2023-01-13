#pragma once
#include <cstdint>
#include <cassert>
#include <cstring>
#include <istream>
#include <sstream>


/** Intel HEX format support. 
 
    For details about the format, see https://en.wikipedia.org/wiki/Intel_HEX
 */
namespace hex {

    /** Record kind. 
     
        Data, end of file, or some of the address specifiers. 
     */
    enum class RecordKind : uint8_t {
        Data = 0, 
        EndOfFile = 1, 
        ExtendedSegmentAddress = 2, 
        StartSegmentAddress = 3,
        ExtendedLinearAddress = 4, 
        StartLinearAddress = 5,
    }; // hex::RecordKind

    /** A record in the Intel HEX file. 
     */
    class Record {
    public:
        class Parser;


        Record() = default;

        ~Record() {
            delete [] data_;
        }

        RecordKind kind() const { return kind_; }

        uint8_t length() const { return length_; }

        uint16_t address() const { return address_; }

        uint8_t const * data() const {
            assert(kind_ == RecordKind::Data && "Attempting to get data of a non-data record");
            return data_;
        }

    private:
        uint8_t length_ = 0;
        uint16_t address_ = 0;
        RecordKind kind_ = RecordKind::EndOfFile;
        uint8_t * data_ = nullptr;

    }; // hex::Record

    class Record::Parser {
    public:

        Parser(std::istream & s):
            s_{s} {
        }

        /** Parses next record from the given stream. 
         
            Returns true if there was new record available, false if eof. Throws an exception if the record is invalid. 

            Record format is `:LLAAAARR[DD]CC` where:

            - `LL` is the lengt of the data field
            - `AAAA` is 16bit address
            - `RR` is record type (see RecordKind)
            - `[DD]` is LL times data byte
            - `CC` is the checksum, a two's complement of all the decoded preceding bytes least significant byte
        */
        bool parseRecord(Record & r) {
            skipWhitespace();
            if (s_.eof())
                return false;
            uint8_t len = parseByte();
            if (len != r.length_) {
                delete r.data_;
                r.data_ = new uint8_t[len];
                r.length_ = len;
            }
            uint16_t addr = static_cast<uint16_t>(parseByte()) << 8 | parseByte();
            uint8_t k = parseByte();
            if (k > static_cast<uint8_t>(RecordKind::StartLinearAddress))
                throw "Invalid record kind";
            r.kind_ = static_cast<RecordKind>(k);
            for (uint8_t i = 0; i < len; ++i) 
                r.data_[i] = parseByte();
            uint8_t checksum = crc_;
            if (checksum != parseByte())
                throw "Invalid CRC";
            return true;
        }

    private:

        void skipWhitespace() {
            while (! s_.eof()) {
                switch (s_.get()) {
                    // we have read the start character, end of whitespace, start of record data
                    case ':':
                        ++c_;
                        crc_ = 0; // reset the crc
                        return;
                    case '\n':
                        ++l_;
                        c_ = 1;
                        break;
                    default:
                        ++c_;
                }
            }
        }

        uint8_t parseByte() {
            uint8_t result = fromHex(s_.get()) << 8 | fromHex(s_.get());
            crc_ += result;
            return result;
        }

        uint8_t fromHex(char x) {
            if (x >= '0' && x <= '9')
                return x - '0';
            else if (x >= 'a' && x <= 'f')
                return x - 'a' + 10;
            else if (x >= 'A' && x <= 'F')
                return x - 'A' + 10;
            else
                throw std::invalid_argument("Not a hex digit");
        }

        // location information for better errors
        size_t l_ = 1;
        size_t c_ = 1;

        uint8_t crc_ = 0;

        std::istream & s_;
    }; // hex::Record::Parser



    struct Program {
        size_t address = 0;
        size_t size = 0;
        uint8_t * data;

        ~Program() {
            delete [] data;
        }


        size_t start() const { return address; }
        size_t end() const { return address + size; }

        static Program parse(std::istream & s) {
            auto parser = Record::Parser{s};
            size_t c = 1024;
            Program p;            
            Record r;
            while (parser.parseRecord(r)) {
                switch (r.kind()) {
                    case RecordKind::Data: {
                        while (p.size + r.length() > c)
                            p.grow();
                        if (r.address() != p.end())
                            throw "Address mismatch";
                        std::memcpy(p.data + p.size, r.data(), r.length());
                        p.size += r.length();
                    }
                    case RecordKind::EndOfFile:
                        return p;
                    default:
                        throw "Unknown record type";
                }
            }
            throw "No end of file record found.";
        }

        static Program parse(char const * s) {
            std::stringstream ss{s};
            return parse(ss);

        }
    private:

        Program():
            data{new uint8_t[c_]} {
        }

        void grow() {
            c_ = c_ * 2;
            uint8_t * d = new uint8_t[c_];
            std::memcpy(d, data, size);
            delete [] data;
            data = d;
        }

        size_t c_ = 1024;

    }; // hex::Program

} // namespace hex



#ifdef TESTS


#endif
