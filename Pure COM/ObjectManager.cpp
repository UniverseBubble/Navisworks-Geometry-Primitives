#include "stdafx.h"
#include "ObjectManager.h"

NvObject::NvObject()
{

}

void NvObject::addProperty(const std::wstring& name, const std::wstring& value, const std::wstring& unit /*= L""*/)
{
	PropertyPtr prop = std::make_shared<Property>(name, value, unit);
	properties.emplace(name, prop);
}

const std::wstring& NvObject::getPropertyValue(const std::wstring& name)
{
	PropertyPtr prop;
	if (!tryGetProperty(name, prop))
	{
		prop = std::make_shared<Property>();
		prop->name = name;
		properties.emplace(name, prop);
	}
	return prop->value;
}

bool NvObject::tryGetPropertyValue(const std::wstring& name, std::wstring& value) const
{
	PropertyPtr prop;
	if (!tryGetProperty(name, prop))
		return false;
	value = prop->value;
	return true;
}

bool NvObject::tryGetProperty(const std::wstring& name, PropertyPtr& prop) const
{
	auto iter = properties.find(name);
	if (iter == properties.end())
		return false;
	prop = iter->second;
	return true;
}


ObjectManager::ObjectManager()
{

}

ObjectPtr ObjectManager::getOrCreateComponent(const std::wstring& id)
{
	auto iter = m_comps.find(id);
	if (iter == m_comps.end())
	{
		auto comp = std::make_shared<NvObject>();
		comp->id = id;
		iter = m_comps.emplace(id, comp).first;
	}
	return iter->second;
}
