#pragma once
#include <cstdint>
#include <cassert>
#include <cstring>
#include <istream>
#include <sstream>

#ifndef STR
#define STR(__VA_ARGS__...) static_cast<std::stringstream &&>(std::stringstream() << __VA_ARGS__).str()
#endif


/** Intel HEX format support. 
 
    For details about the format, see https://en.wikipedia.org/wiki/Intel_HEX
 */
namespace hex {

    struct Error : std::invalid_argument {
        size_t line;
        size_t col;

        Error(std::string const & str, size_t line, size_t col):
            std::invalid_argument{str},
            line{line},
            col{col} {
        }

        friend std::ostream & operator << (std::ostream & s, Error const & e) {
            s << e.what() << " (line " << e.line << ", col " << e.col << ")";
            return s;
        }

    }; // hex::Error

    /** Record kind. 
     
        Data, end of file, or some of the address specifiers. 
     */
    enum class RecordKind : uint8_t {
        /** Data. Up to 256 bytes per record is supported. 
         */
        Data = 0, 
        /** End of file. Required for proper parsing.
         */
        EndOfFile = 1, 
        ExtendedSegmentAddress = 2, 
        /** 4byte address where the code execution should start.
         */
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

        size_t length() const { return length_; }

        size_t address() const { 
            switch (kind_) {
                case RecordKind::Data:
                    return address_;
                case RecordKind::StartSegmentAddress:
                    return (data_[0] << 24) | (data_[1] << 16) | (data_[2] << 8) | data_[3];
                default:
                    assert(false && "Cannot take address of a given record kind");
            }
        }

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
            r.address_ = static_cast<uint16_t>(parseByte()) << 8 | parseByte();
            uint8_t k = parseByte();
            if (k > static_cast<uint8_t>(RecordKind::StartLinearAddress))
                throw Error{STR("Invalid record kind: " << (int)k), l_, c_};
            r.kind_ = static_cast<RecordKind>(k);
            for (uint8_t i = 0; i < len; ++i) 
                r.data_[i] = parseByte();
            uint8_t crc = (~crc_ + 1) & 0xff;
            uint8_t checksum = parseByte();
            if (crc != checksum)
                throw Error{STR("Invalid CRC: expected " << (int)crc << ", found " << (int)checksum), l_, c_};
            return true;
        }

    private:

        friend class Program;

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
            uint8_t result = (fromHex(s_.get()) << 4) | fromHex(s_.get());
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
                throw Error{STR("Not a hex digit: " << x << " (code: " << (int)x << ")"), l_, c_};
        }

        // location information for better errors
        size_t l_ = 1;
        size_t c_ = 1;

        unsigned crc_ = 0;

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
            Program p;            
            Record r;
            size_t line = parser.l_;
            size_t col = parser.c_;
            while (parser.parseRecord(r)) {
                switch (r.kind()) {
                    case RecordKind::Data: {
                        while (p.size + r.length() > p.c_)
                            p.grow();
                        if (p.size == 0)
                            p.address = r.address();
                        else if (r.address() != p.end())
                            throw Error{STR("Address mismatch: program expects " << p.end() << ", record specifies " << r.address()), line, col};
                        std::memcpy(p.data + p.size, r.data(), r.length());
                        p.size += r.length();
                        break;
                    }
                    case RecordKind::EndOfFile:
                        return p;
                    case RecordKind::StartSegmentAddress:
                        assert(p.address = r.address());
                        break;
                    default:
                        throw Error{STR("Unsupported record type: " << (int) r.kind()), line, col};
                }
                line = parser.l_;
                col = parser.c_;
            }
            throw Error{"No end of file record found.", line, col};
        }

        static Program parse(char const * s) {
            std::stringstream ss{s};
            return parse(ss);

        }
    private:

        Program():
            data{new uint8_t[1024]} {
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
