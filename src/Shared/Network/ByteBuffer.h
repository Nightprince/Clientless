/*
 * Copyright (C) 2015 Dehravor <dehravor@gmail.com>
 * Copyright (C) 2008-2014 TrinityCore <http://www.trinitycore.org/>
 * Copyright (C) 2005-2009 MaNGOS <http://getmangos.com/>
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#pragma once

#include "Define.h"
#include "Common.h"
#include "ByteConverter.h"
#include "World/ObjectGuid.h"

#include <iostream>
#include <exception>
#include <list>
#include <map>
#include <string>
#include <vector>
#include <cstring>
#include <time.h>

#if _MSC_VER
    #pragma warning(disable:4996)
#endif

// Root of ByteBuffer exception hierarchy
class ByteBufferException : public std::exception
{
public:
    ~ByteBufferException() throw() { }

    char const* what() const throw() { return msg_.c_str(); }

protected:
    std::string & message() throw() { return msg_; }

private:
    std::string msg_;
};

class ByteBufferPositionException : public ByteBufferException
{
public:
    ByteBufferPositionException(bool add, size_t pos, size_t size, size_t valueSize);

    ~ByteBufferPositionException() throw() { }
};

class ByteBufferSourceException : public ByteBufferException
{
public:
    ByteBufferSourceException(size_t pos, size_t size, size_t valueSize);

    ~ByteBufferSourceException() throw() { }
};

class ByteBuffer
{
    public:
        const static size_t DEFAULT_SIZE = 0x1000;

        // constructor
        ByteBuffer() : rpos_(0), wpos_(0), bitpos_(8), curbitval_(0)
        {
            storage_.reserve(DEFAULT_SIZE);
        }

        ByteBuffer(size_t reserve) : rpos_(0), wpos_(0), bitpos_(8), curbitval_(0)
        {
            storage_.reserve(reserve);
        }

        // copy constructor
        ByteBuffer(const ByteBuffer &buf) : rpos_(buf.rpos_), wpos_(buf.wpos_),
            storage_(buf.storage_), bitpos_(buf.bitpos_), curbitval_(buf.curbitval_)
        {
        }

        void clear()
        {
            storage_.clear();
            rpos_ = wpos_ = 0;
        }

        template <typename T> void append(T value)
        {
            EndianConvert(value);
            append((uint8 *)&value, sizeof(value));
        }

        void FlushBits()
        {
            if (bitpos_ == 8)
                return;

            append((uint8 *)&curbitval_, sizeof(uint8));
            curbitval_ = 0;
            bitpos_ = 8;
        }

        bool WriteBit(uint32 bit)
        {
            --bitpos_;
            if (bit)
                curbitval_ |= (1 << (bitpos_));

            if (bitpos_ == 0)
            {
                bitpos_ = 8;
                append((uint8 *)&curbitval_, sizeof(curbitval_));
                curbitval_ = 0;
            }

            return (bit != 0);
        }

        bool ReadBit()
        {
            ++bitpos_;
            if (bitpos_ > 7)
            {
                bitpos_ = 0;
                curbitval_ = read<uint8>();
            }

            return ((curbitval_ >> (7 - bitpos_)) & 1) != 0;
        }

        template <typename T> void WriteBits(T value, size_t bits)
        {
            for (int32 i = bits - 1; i >= 0; --i)
                WriteBit((value >> i) & 1);
        }

        uint32 ReadBits(size_t bits)
        {
            uint32 value = 0;
            for (int32 i = bits - 1; i >= 0; --i)
            if (ReadBit())
                value |= (1 << (i));

            return value;
        }

        void ReadByteSeq(uint8& b)
        {
            if (b != 0)
                b ^= read<uint8>();
        }

        void WriteByteSeq(uint8 b)
        {
            if (b != 0)
                append<uint8>(b ^ 1);
        }

        template <typename T> void put(size_t pos, T value)
        {
            EndianConvert(value);
            put(pos, (uint8 *)&value, sizeof(value));
        }

        template <typename T> void PutBits(size_t pos, T value, uint32 bitCount)
        {
            if (!bitCount)
                throw ByteBufferSourceException((pos + bitCount) / 8, size(), 0);

            if (pos + bitCount > size() * 8)
                throw ByteBufferPositionException(false, (pos + bitCount) / 8, size(), (bitCount - 1) / 8 + 1);

            for (uint32 i = 0; i < bitCount; ++i)
            {
                size_t wp = (pos + i) / 8;
                size_t bit = (pos + i) % 8;
                if ((value >> (bitCount - i - 1)) & 1)
                    storage_[wp] |= 1 << (7 - bit);
                else
                    storage_[wp] &= ~(1 << (7 - bit));
            }
        }

        std::string ReadString(uint32 length)
        {
            if (!length)
                return std::string();
            char* buffer = new char[length + 1]();
            read((uint8*)buffer, length);
            std::string retval = buffer;
            delete[] buffer;
            return retval;
        }

        void WriteString(std::string const& str)
        {
            if (size_t len = str.length())
                append(str.c_str(), len);
        }

        ByteBuffer &operator<<(uint8 value)
        {
            append<uint8>(value);
            return *this;
        }

        ByteBuffer &operator<<(uint16 value)
        {
            append<uint16>(value);
            return *this;
        }

        ByteBuffer &operator<<(uint32 value)
        {
            append<uint32>(value);
            return *this;
        }

        ByteBuffer &operator<<(uint64 value)
        {
            append<uint64>(value);
            return *this;
        }

        // signed as in 2e complement
        ByteBuffer &operator<<(int8 value)
        {
            append<int8>(value);
            return *this;
        }

        ByteBuffer &operator<<(int16 value)
        {
            append<int16>(value);
            return *this;
        }

        ByteBuffer &operator<<(int32 value)
        {
            append<int32>(value);
            return *this;
        }

        ByteBuffer &operator<<(int64 value)
        {
            append<int64>(value);
            return *this;
        }

        // floating points
        ByteBuffer &operator<<(float value)
        {
            append<float>(value);
            return *this;
        }

        ByteBuffer &operator<<(double value)
        {
            append<double>(value);
            return *this;
        }

        ByteBuffer &operator<<(const std::string &value)
        {
            if (size_t len = value.length())
                append((uint8 const*)value.c_str(), len);
            append((uint8)0);
            return *this;
        }

        ByteBuffer &operator<<(const char *str)
        {
            if (size_t len = (str ? strlen(str) : 0))
                append((uint8 const*)str, len);
            append((uint8)0);
            return *this;
        }

        ByteBuffer &operator>>(bool &value)
        {
            value = read<char>() > 0 ? true : false;
            return *this;
        }

        ByteBuffer &operator>>(uint8 &value)
        {
            value = read<uint8>();
            return *this;
        }

        ByteBuffer &operator>>(uint16 &value)
        {
            value = read<uint16>();
            return *this;
        }

        ByteBuffer &operator>>(uint32 &value)
        {
            value = read<uint32>();
            return *this;
        }

        ByteBuffer &operator>>(uint64 &value)
        {
            value = read<uint64>();
            return *this;
        }

        //signed as in 2e complement
        ByteBuffer &operator>>(int8 &value)
        {
            value = read<int8>();
            return *this;
        }

        ByteBuffer &operator>>(int16 &value)
        {
            value = read<int16>();
            return *this;
        }

        ByteBuffer &operator>>(int32 &value)
        {
            value = read<int32>();
            return *this;
        }

        ByteBuffer &operator>>(int64 &value)
        {
            value = read<int64>();
            return *this;
        }

        ByteBuffer &operator>>(float &value)
        {
            value = read<float>();
            return *this;
        }

        ByteBuffer &operator>>(double &value)
        {
            value = read<double>();
            return *this;
        }

        ByteBuffer &operator>>(std::string& value)
        {
            value.clear();
            while (rpos() < size())                         // prevent crash at wrong string format in packet
            {
                char c = read<char>();
                if (c == 0)
                    break;
                value += c;
            }
            return *this;
        }

        ByteBuffer &operator>>(ObjectGuid& guid)
        {
            guid = read<uint64_t>();
            return *this;
        }

        uint8& operator[](size_t const pos)
        {
            if (pos >= size())
                throw ByteBufferPositionException(false, pos, 1, size());
            return storage_[pos];
        }

        uint8 const& operator[](size_t const pos) const
        {
            if (pos >= size())
                throw ByteBufferPositionException(false, pos, 1, size());
            return storage_[pos];
        }

        size_t rpos() const { return rpos_; }

        size_t rpos(size_t rpos_)
        {
            rpos_ = rpos_;
            return rpos_;
        }

        void rfinish()
        {
            rpos_ = wpos();
        }

        size_t wpos() const { return wpos_; }

        size_t wpos(size_t wpos_)
        {
            wpos_ = wpos_;
            return wpos_;
        }

        template<typename T>
        void read_skip() { read_skip(sizeof(T)); }

        void read_skip(size_t skip)
        {
            if (rpos_ + skip > size())
                throw ByteBufferPositionException(false, rpos_, skip, size());
            rpos_ += skip;
        }

        template <typename T> T read()
        {
            T r = read<T>(rpos_);
            rpos_ += sizeof(T);
            return r;
        }

        template <typename T> T read(size_t pos) const
        {
            if (pos + sizeof(T) > size())
                throw ByteBufferPositionException(false, pos, sizeof(T), size());
            T val = *((T const*)&storage_[pos]);
            EndianConvert(val);
            return val;
        }

        void read(uint8 *dest, size_t len)
        {
            if (rpos_  + len > size())
               throw ByteBufferPositionException(false, rpos_, len, size());
            std::memcpy(dest, &storage_[rpos_], len);
            rpos_ += len;
        }

        void readPackGUID(uint64_t& guid)
        {
            if (rpos() + 1 > size())
                throw ByteBufferPositionException(false, rpos_, 1, size());

            guid = 0;

            uint8_t guidmark = 0;
            (*this) >> guidmark;

            for (int i = 0; i < 8; ++i)
            {
                if (guidmark & (uint8_t(1) << i))
                {
                    if (rpos() + 1 > size())
                        throw ByteBufferPositionException(false, rpos_, 1, size());

                    uint8_t bit;
                    (*this) >> bit;
                    guid |= (uint64_t(bit) << (i * 8));
                }
            }
        }

        uint32 ReadPackedTime()
        {
            uint32 packedDate = read<uint32>();
            tm lt = tm();

            lt.tm_min = packedDate & 0x3F;
            lt.tm_hour = (packedDate >> 6) & 0x1F;
            //lt.tm_wday = (packedDate >> 11) & 7;
            lt.tm_mday = ((packedDate >> 14) & 0x3F) + 1;
            lt.tm_mon = (packedDate >> 20) & 0xF;
            lt.tm_year = ((packedDate >> 24) & 0x1F) + 100;

            return uint32(mktime(&lt) + timezone);
        }

        ByteBuffer& ReadPackedTime(uint32& time)
        {
            time = ReadPackedTime();
            return *this;
        }

        uint64 ReadObjectGuid()
        {
            return read<uint64>();
        }

        uint8 * contents() { return &storage_[0]; }

        const uint8 *contents() const { return &storage_[0]; }

        size_t size() const { return storage_.size(); }
        bool empty() const { return storage_.empty(); }

        void resize(size_t newsize)
        {
            storage_.resize(newsize, 0);
            rpos_ = 0;
            wpos_ = size();
        }

        void reserve(size_t ressize)
        {
            if (ressize > size())
                storage_.reserve(ressize);
        }

        void drop(uint32 length)
        {
            storage_.erase(storage_.begin(), storage_.begin() + length);
            rpos_ = 0;
            wpos_ = size();
        }

        void append(const char *src, size_t cnt)
        {
            return append((const uint8 *)src, cnt);
        }

        template<class T> void append(const T *src, size_t cnt)
        {
            return append((const uint8 *)src, cnt * sizeof(T));
        }

        void append(const uint8 *src, size_t cnt)
        {
            if (!cnt)
                throw ByteBufferSourceException(wpos_, size(), cnt);

            if (!src)
                throw ByteBufferSourceException(wpos_, size(), cnt);

            assert(size() < 10000000);

            if (storage_.size() < wpos_ + cnt)
                storage_.resize(wpos_ + cnt);
            std::memcpy(&storage_[wpos_], src, cnt);
            wpos_ += cnt;
        }

        void append(const ByteBuffer& buffer)
        {
            if (buffer.wpos())
                append(buffer.contents(), buffer.wpos());
        }

        // can be used in SMSG_MONSTER_MOVE opcode
        void appendPackXYZ(float x, float y, float z)
        {
            uint32 packed = 0;
            packed |= ((int)(x / 0.25f) & 0x7FF);
            packed |= ((int)(y / 0.25f) & 0x7FF) << 11;
            packed |= ((int)(z / 0.25f) & 0x3FF) << 22;
            *this << packed;
        }

        void appendPackGUID(uint64 guid)
        {
            uint8 packGUID[8+1];
            packGUID[0] = 0;
            size_t size = 1;
            for (uint8 i = 0;guid != 0;++i)
            {
                if (guid & 0xFF)
                {
                    packGUID[0] |= uint8(1 << i);
                    packGUID[size] =  uint8(guid & 0xFF);
                    ++size;
                }

                guid >>= 8;
            }
            append(packGUID, size);
        }

        void AppendPackedTime(time_t time)
        {
            tm* lt = localtime(&time);
            append<uint32>((lt->tm_year - 100) << 24 | lt->tm_mon  << 20 | (lt->tm_mday - 1) << 14 | lt->tm_wday << 11 | lt->tm_hour << 6 | lt->tm_min);
        }

        void put(size_t pos, const uint8 *src, size_t cnt)
        {
            if (pos + cnt > size())
                throw ByteBufferPositionException(true, pos, cnt, size());

            if (!src)
                throw ByteBufferSourceException(wpos_, size(), cnt);

            std::memcpy(&storage_[pos], src, cnt);
        }

        void print_storage() const;

        void textlike() const;

        void hexlike() const;

    protected:
        size_t rpos_, wpos_, bitpos_;
        uint8 curbitval_;
        std::vector<uint8> storage_;
};

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::vector<T> v)
{
    b << (uint32)v.size();
    for (typename std::vector<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::vector<T> &v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator<<(ByteBuffer &b, std::list<T> v)
{
    b << (uint32)v.size();
    for (typename std::list<T>::iterator i = v.begin(); i != v.end(); ++i)
    {
        b << *i;
    }
    return b;
}

template <typename T>
inline ByteBuffer &operator>>(ByteBuffer &b, std::list<T> &v)
{
    uint32 vsize;
    b >> vsize;
    v.clear();
    while (vsize--)
    {
        T t;
        b >> t;
        v.push_back(t);
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer &operator<<(ByteBuffer &b, std::map<K, V> &m)
{
    b << (uint32)m.size();
    for (typename std::map<K, V>::iterator i = m.begin(); i != m.end(); ++i)
    {
        b << i->first << i->second;
    }
    return b;
}

template <typename K, typename V>
inline ByteBuffer &operator>>(ByteBuffer &b, std::map<K, V> &m)
{
    uint32 msize;
    b >> msize;
    m.clear();
    while (msize--)
    {
        K k;
        V v;
        b >> k >> v;
        m.insert(make_pair(k, v));
    }
    return b;
}

template<> inline std::string ByteBuffer::read<std::string>()
{
    std::string tmp;
    *this >> tmp;
    return tmp;
}

template<>
inline void ByteBuffer::read_skip<char*>()
{
    std::string temp;
    *this >> temp;
}

template<>
inline void ByteBuffer::read_skip<char const*>()
{
    read_skip<char*>();
}

template<>
inline void ByteBuffer::read_skip<std::string>()
{
    read_skip<char*>();
}
