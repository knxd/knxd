#ifndef ADDRESSFILTER_H
#define ADDRESSFILTER_H

#include "link.h"
#include "inifile.h"
#include "eibtypes.h"
#include <set>
#include <string>

// 使用 FILTER 宏声明类，绑定名称 "address"
FILTER(AddressFilter, address) {
public:
    // 构造函数（参数需与基类 Filter 一致）
    AddressFilter(LinkConnectPtr link, IniSectionPtr config);
    ~AddressFilter() override = default;

    // 重写接收和发送过滤方法
    bool recv(LDataPtr frame) override;
    bool send(LDataPtr frame) override;

private:
    // 地址解析方法
    eibaddr_t parsePhysicalAddress(const std::string &addr);
    eibaddr_t parseGroupAddress(const std::string &addr);

    // 解析配置
    void parseConfig(IniSectionPtr config);

    // 地址检查方法
    bool checkPhysicalAddress(eibaddr_t physAddr);
    bool checkGroupAddress(eibaddr_t groupAddr);

private:
    // 存储允许/拒绝的地址集合
    std::set<eibaddr_t> allowPhysical_;
    std::set<eibaddr_t> denyPhysical_;
    std::set<eibaddr_t> allowGroup_;
    std::set<eibaddr_t> denyGroup_;
};


#endif // ADDRESSFILTER_H