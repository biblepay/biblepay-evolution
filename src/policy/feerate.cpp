// Copyright (c) 2009-2010 Satoshi Nakamoto
// Copyright (c) 2009-2015 The Bitcoin Core developers
// Distributed under the MIT software license, see the accompanying
// file COPYING or http://www.opensource.org/licenses/mit-license.php.

#include <policy/feerate.h>

#include <tinyformat.h>
#include "util.h"

const std::string CURRENCY_UNIT = "BIBLEPAY";

CFeeRate::CFeeRate(const CAmount& nFeePaid, size_t nBytes_)
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);

    if (nSize > 0)
	{
        nSatoshisPerK = nFeePaid * 1000 / nSize;
	}
    else
	{
        nSatoshisPerK = 0;
	}
}

static const unsigned int MIN_RELAY_TX_FEE1 = 100007777;

CAmount CFeeRate::GetFee(size_t nBytes_) const
{
    assert(nBytes_ <= uint64_t(std::numeric_limits<int64_t>::max()));
    int64_t nSize = int64_t(nBytes_);
	// BiblePay fees must be more significant:
	// Note: Users may need to delete feerate.dat to allow recalculation to occur
	unsigned int nSatPerK = nSatoshisPerK;
	if (nSatPerK > MIN_RELAY_TX_FEE1 * 1.25)
	{
		nSatPerK = MIN_RELAY_TX_FEE1 * 1.25;
	}

    CAmount nFee = nSatPerK * nSize / 1000;
    if (false)
        LogPrintf("\nBytes %f, SatPerK %f, Size %f, fee %s", nBytes_, nSatoshisPerK, nSize, (double)nFee/COIN);

    if (nFee == 0 && nSize != 0) {
        if (nSatoshisPerK > 0)
            nFee = CAmount(1);
        if (nSatoshisPerK < 0)
            nFee = CAmount(-1);
    }

    return nFee;
}

std::string CFeeRate::ToString() const
{
    return strprintf("%d.%08d %s/kB", nSatoshisPerK / COIN, nSatoshisPerK % COIN, CURRENCY_UNIT);
}
