/*
    knxd EIB KNX daemon
    Copyright (C) 2026 Kwangseob Jeong <myddpp@naver.com>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "groupfilter.h"

#include <sstream>

GroupFilter::GroupFilter(const LinkConnectPtr_ &c, IniSectionPtr &s)
    : Filter(c, s)
{
}

bool GroupFilter::setup()
{
    if (!Filter::setup())
        return false;

    // Parse mode: "allow" or "block" (default)
    std::string modeStr = cfg->value("mode", "block");
    if (modeStr == "allow")
        mode_ = Mode::ALLOW;
    else if (modeStr == "block")
        mode_ = Mode::BLOCK;
    else
    {
        ERRORPRINTF(t, E_ERROR | 120, "groupfilter: unknown mode '%s', expected 'allow' or 'block'", modeStr.c_str());
        return false;
    }

    // Parse comma-separated group addresses
    std::string addrList = cfg->value("addresses", "");
    if (addrList.empty())
    {
        ERRORPRINTF(t, E_WARNING | 121, "groupfilter: no addresses configured");
        return true;
    }

    std::istringstream ss(addrList);
    std::string token;
    while (std::getline(ss, token, ','))
    {
        // Trim whitespace
        size_t start = token.find_first_not_of(" \t");
        size_t end = token.find_last_not_of(" \t");
        if (start == std::string::npos)
            continue;
        token = token.substr(start, end - start + 1);

        eibaddr_t addr;
        if (!parseGroupAddress(token, addr))
        {
            ERRORPRINTF(t, E_ERROR | 122, "groupfilter: invalid address '%s'", token.c_str());
            return false;
        }
        addresses_.insert(addr);
    }

    TRACEPRINTF(t, 1, "groupfilter: mode=%s, %d address(es)",
                modeStr.c_str(), (int)addresses_.size());
    return true;
}

bool GroupFilter::parseGroupAddress(const std::string &str, eibaddr_t &addr)
{
    unsigned int a, b, c;
    if (sscanf(str.c_str(), "%u/%u/%u", &a, &b, &c) == 3)
    {
        if (a > 31 || b > 7 || c > 255)
            return false;
        addr = (eibaddr_t)((a << 11) | (b << 8) | c);
        return true;
    }
    // Also support 2-level: a/b
    if (sscanf(str.c_str(), "%u/%u", &a, &b) == 2)
    {
        if (a > 31 || b > 2047)
            return false;
        addr = (eibaddr_t)((a << 11) | b);
        return true;
    }
    return false;
}

bool GroupFilter::shouldPass(const L_Data_PDU &pdu) const
{
    // Only filter group-addressed telegrams
    if (pdu.address_type != GroupAddress)
        return true;

    bool found = addresses_.count(pdu.destination_address) > 0;

    if (mode_ == Mode::ALLOW)
        return found; // allow mode: pass only if in the set
    else
        return !found; // block mode: pass only if NOT in the set
}

void GroupFilter::send_L_Data(LDataPtr l)
{
    if (shouldPass(*l))
        Filter::send_L_Data(std::move(l));
    else
    {
        TRACEPRINTF(t, 1, "groupfilter: blocked send to %s",
                    FormatGroupAddr(l->destination_address));
        send_Next();
    }
}

void GroupFilter::recv_L_Data(LDataPtr l)
{
    if (shouldPass(*l))
        Filter::recv_L_Data(std::move(l));
    else
    {
        TRACEPRINTF(t, 1, "groupfilter: blocked recv from %s to %s",
                    FormatEIBAddr(l->source_address),
                    FormatGroupAddr(l->destination_address));
    }
}
