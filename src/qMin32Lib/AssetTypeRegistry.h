#pragma once

#include "IAsset.h"

class AssetTypeRegistry
{
public:
    using Factory = std::function<std::unique_ptr<IAsset>(std::string_view)>;

    bool Register(AssetTypeId type, std::string_view name, std::string_view ext, Factory factory)
    {
        if (!type || ext.empty() || !factory)
            return false;
        AssetTypeDesc desc;
        desc.type = type;
        desc.name = std::string(name);
        desc.ext = NormalizeExt(ext);
        desc.factory = std::move(factory);
        std::unique_lock lock(m_mutex);
        m_byType[type] = desc;
        m_byExt[desc.ext] = type;
        return true;
    }

    std::unique_ptr<IAsset> CreateByPath(std::string_view path) const
    {
        const auto ext = ExtractExt(path);
        std::shared_lock lock(m_mutex);
        auto extIt = m_byExt.find(ext);
        if (extIt == m_byExt.end())
            return {};
        auto typeIt = m_byType.find(extIt->second);
        if (typeIt == m_byType.end())
            return {};
        return typeIt->second.factory(path);
    }

    AssetTypeId FindTypeByPath(std::string_view path) const
    {
        const auto ext = ExtractExt(path);
        std::shared_lock lock(m_mutex);
        auto it = m_byExt.find(ext);
        return it == m_byExt.end() ? 0u : it->second;
    }

private:
    struct AssetTypeDesc
    {
        AssetTypeId type = 0;
        std::string name;
        std::string ext;
        Factory factory;
    };

    static std::string NormalizeExt(std::string_view ext)
    {
        std::string out(ext);
        if (!out.empty() && out.front() == '.')
            out.erase(out.begin());
        std::transform(out.begin(), out.end(), out.begin(), [](unsigned char ch) { return static_cast<char>(std::tolower(ch)); });
        return out;
    }

    static std::string ExtractExt(std::string_view path)
    {
        const auto pos = path.find_last_of('.');
        return pos == std::string_view::npos ? std::string() : NormalizeExt(path.substr(pos + 1));
    }

    mutable std::shared_mutex m_mutex;
    std::unordered_map<AssetTypeId, AssetTypeDesc> m_byType;
    std::unordered_map<std::string, AssetTypeId> m_byExt;
};
