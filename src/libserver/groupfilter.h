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

#ifndef GROUPFILTER_H
#define GROUPFILTER_H

#include <unordered_set>
#include "link.h"

/**
 * @brief Filter that allows or blocks KNX group address telegrams.
 *
 * Configuration (in knxd.ini section):
 *   filter = group
 *   mode = allow|block    (default: block)
 *   addresses = 0/0/1,1/2/3,2/1/0   (comma-separated group addresses)
 *
 * In "block" mode, telegrams to listed addresses are dropped.
 * In "allow" mode, only telegrams to listed addresses are passed through.
 */
RFILTER(GroupFilter, group)
{
public:
    GroupFilter(const LinkConnectPtr_ &c, IniSectionPtr &s);
    virtual ~GroupFilter() = default;

    bool setup() override;

    void send_L_Data(LDataPtr l) override;
    void recv_L_Data(LDataPtr l) override;

private:
    enum class Mode
    {
        ALLOW,
        BLOCK
    };

    Mode mode_ = Mode::BLOCK;
    std::unordered_set<eibaddr_t> addresses_;

    /** Parse "a/b/c" format to 16-bit group address */
    static bool parseGroupAddress(const std::string &str, eibaddr_t &addr);

    /** Check whether a packet should pass through */
    bool shouldPass(const L_Data_PDU &pdu) const;
};

#endif /* GROUPFILTER_H */
