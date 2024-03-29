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
#include <memory>

struct bignum_st;

class BigNumber
{
    public:
        BigNumber();
        BigNumber(BigNumber const& bn);
        BigNumber(uint32 val);
        BigNumber(uint8 const* buffer, int32 length);
        ~BigNumber();

        void SetZero();
        void SetOne();
        void SetUInt32(uint32 value);
        void SetUInt64(uint64 value);
        void SetBinary(uint8 const* bytes, int32 len);
        void SetRandom(int32 length);
        void SetHexString(const char* str);

        void Negate();
        bool IsNegative();
        bool IsZero();
        bool IsOne();
        bool IsOdd();
        bool IsEven();

        bool operator==(BigNumber const& bn);
        bool operator>(BigNumber const& bn);
        bool operator<(BigNumber const& bn);

        BigNumber& operator=(BigNumber const& bn);

        BigNumber operator+=(BigNumber const& bn);
        BigNumber operator+(BigNumber const& bn)
        {
            BigNumber t(*this);
            return t += bn;
        }

        BigNumber operator-=(BigNumber const& bn);
        BigNumber operator-(BigNumber const& bn)
        {
            BigNumber t(*this);
            return t -= bn;
        }

        BigNumber operator*=(BigNumber const& bn);
        BigNumber operator*(BigNumber const& bn)
        {
            BigNumber t(*this);
            return t *= bn;
        }

        BigNumber operator/=(BigNumber const& bn);
        BigNumber operator/(BigNumber const& bn)
        {
            BigNumber t(*this);
            return t /= bn;
        }

        BigNumber operator%=(BigNumber const& bn);
        BigNumber operator%(BigNumber const& bn)
        {
            BigNumber t(*this);
            return t %= bn;
        }

        bool isZero() const;

        BigNumber ModExp(BigNumber const& bn1, BigNumber const& bn2);
        BigNumber Exp(BigNumber const&);

        int32 GetNumBytes(void) const;

        struct bignum_st* BN() { return bn_; }

        uint32 AsDword();

        std::unique_ptr<uint8[]> AsByteArray(int32 minSize = 0, bool littleEndian = true) const;

        char* AsHexStr() const;
        char* AsDecStr() const;

    private:
        struct bignum_st *bn_;
};

