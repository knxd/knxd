#include "addressfilter.h"
#include "common.h"
#include "error.h"
#include <sstream>
#include <cstring>

// 实现构造函数
AddressFilter::AddressFilter(LinkConnectPtr link, IniSectionPtr config)
    : Filter(link, config) {
    parseConfig(config);
    TRACEPRINTF(t, 4, "AddressFilter (FILTER macro) initialized");
}

// 解析配置（与之前逻辑一致）
void AddressFilter::parseConfig(IniSectionPtr config) {
    // 解析 allow-physical
    std::string allowPhys = config->value("allow-physical", "");
    std::istringstream issPhys(allowPhys);
    std::string addr;
    while (std::getline(issPhys, addr, ',')) {
        if (!addr.empty()) {
            try {
                eibaddr_t phys = parsePhysicalAddress(addr);
                allowPhysical_.insert(phys);
                TRACEPRINTF(t, 4, "Allow physical: %s", addr.c_str());
            } catch (...) {
                ERRORPRINTF(t, E_WARNING, "Invalid physical address: %s", addr.c_str());
            }
        }
    }

    // 解析 deny-physical
    std::string denyPhys = config->value("deny-physical", "");
    std::istringstream issDenyPhys(denyPhys);
    while (std::getline(issDenyPhys, addr, ',')) {
        if (!addr.empty()) {
            try {
                eibaddr_t phys = parsePhysicalAddress(addr);
                denyPhysical_.insert(phys);
                TRACEPRINTF(t, 4, "Deny physical: %s", addr.c_str());
            } catch (...) {
                ERRORPRINTF(t, E_WARNING, "Invalid physical address: %s", addr.c_str());
            }
        }
    }

    // 解析 allow-group
    std::string allowGrp = config->value("allow-group", "");
    std::istringstream issGrp(allowGrp);
    while (std::getline(issGrp, addr, ',')) {
        if (!addr.empty()) {
            try {
                eibaddr_t grp = parseGroupAddress(addr);
                allowGroup_.insert(grp);
                TRACEPRINTF(t, 4, "Allow group: %s", addr.c_str());
            } catch (...) {
                ERRORPRINTF(t, E_WARNING, "Invalid group address: %s", addr.c_str());
            }
        }
    }

    // 解析 deny-group
    std::string denyGrp = config->value("deny-group", "");
    std::istringstream issDenyGrp(denyGrp);
    while (std::getline(issDenyGrp, addr, ',')) {
        if (!addr.empty()) {
            try {
                eibaddr_t grp = parseGroupAddress(addr);
                denyGroup_.insert(grp);
                TRACEPRINTF(t, 4, "Deny group: %s", addr.c_str());
            } catch (...) {
                ERRORPRINTF(t, E_WARNING, "Invalid group address: %s", addr.c_str());
            }
        }
    }
}
// 物理地址解析（与框架对齐）
eibaddr_t AddressFilter::parsePhysicalAddress(const std::string &addr) {
    int a, b, c;
    if (sscanf(addr.c_str(), "%d.%d.%d", &a, &b, &c) != 3) {
        throw std::invalid_argument("Invalid physical address (X.Y.Z)");
    }
    if (a < 0 || a > 15 || b < 0 || b > 15 || c < 0 || c > 255) {
        throw std::invalid_argument("Physical address out of range");
    }
    return ((a & 0x0F) << 12) | ((b & 0x0F) << 8) | (c & 0xFF);
}

// 组地址解析（与框架对齐）
eibaddr_t AddressFilter::parseGroupAddress(const std::string &addr) {
    int a, b, c;
    if (sscanf(addr.c_str(), "%d/%d/%d", &a, &b, &c) != 3) {
        throw std::invalid_argument("Invalid group address (X/Y/Z)");
    }
    if (a < 0 || a > 15 || b < 0 || b > 7 || c < 0 || c > 255) {
        throw std::invalid_argument("Group address out of range");
    }
    return ((a & 0x0F) << 11) | ((b & 0x07) << 8) | (c & 0xFF);
}

// 物理地址检查
bool AddressFilter::checkPhysicalAddress(eibaddr_t physAddr) {
    if (denyPhysical_.count(physAddr)) {
        TRACEPRINTF(t, 3, "Blocked physical: %s", FormatEIBAddr(physAddr).c_str());
        return false;
    }
    if (!allowPhysical_.empty() && !allowPhysical_.count(physAddr)) {
        TRACEPRINTF(t, 3, "Not allowed physical: %s", FormatEIBAddr(physAddr).c_str());
        return false;
    }
    return true;
}

// 组地址检查
bool AddressFilter::checkGroupAddress(eibaddr_t groupAddr) {
    if (denyGroup_.count(groupAddr)) {
        TRACEPRINTF(t, 3, "Blocked group: %s", FormatEIBAddr(groupAddr).c_str());
        return false;
    }
    if (!allowGroup_.empty() && !allowGroup_.count(groupAddr)) {
        TRACEPRINTF(t, 3, "Not allowed group: %s", FormatEIBAddr(groupAddr).c_str());
        return false;
    }
    return true;
}

// 接收过滤
bool AddressFilter::recv(LDataPtr frame) {
    eibaddr_t srcPhys = frame->source_address;
    if (!checkPhysicalAddress(srcPhys)) {
        return false;
    }
    if (frame->address_type == GroupAddress) {
        eibaddr_t groupAddr = frame->destination_address;
        if (!checkGroupAddress(groupAddr)) {
            return false;
        }
    }
    return Filter::recv(frame);
}

// 发送过滤
bool AddressFilter::send(LDataPtr frame) {
    eibaddr_t srcPhys = frame->source_address;
    if (!checkPhysicalAddress(srcPhys)) {
        return false;
    }
    if (frame->address_type == GroupAddress) {
        eibaddr_t groupAddr = frame->destination_address;
        if (!checkGroupAddress(groupAddr)) {
            return false;
        }
    }
    return Filter::send(frame);
}