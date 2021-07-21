#pragma once
#include <string>
#include <memory>
#include <map>
#include <vector>

struct Property;
struct NvObject;
typedef std::shared_ptr<Property> PropertyPtr;
typedef std::map<std::wstring, PropertyPtr> Properties;
typedef std::shared_ptr<NvObject> ObjectPtr;
typedef std::shared_ptr<std::string> DataPtr;

struct Property
{
	std::wstring name;
	std::wstring value;
	std::wstring unit;

public:
	Property()
	{

	}

	Property(const std::wstring& n, const std::wstring& v, const std::wstring& u)
		: name(n), value(v), unit(u)
	{

	}

	Property(const std::wstring& n, const std::wstring& v)
		: Property(n, v, L"")
	{

	}
};


struct NvObject
{
	std::wstring id;
	std::wstring category;
	std::wstring parentId;
	Properties properties;
	std::vector<DataPtr> geometryDatas;

	NvObject();
	void addProperty(const std::wstring& name, const std::wstring& value, const std::wstring& unit = L"");
	const std::wstring& getPropertyValue(const std::wstring& name);
	bool tryGetPropertyValue(const std::wstring& name, std::wstring& value) const;
	bool tryGetProperty(const std::wstring& name, PropertyPtr& prop) const;
};


class ObjectManager
{
public:
	ObjectManager();

	ObjectPtr getOrCreateComponent(const std::wstring& id);
	const std::map<std::wstring, ObjectPtr>& getComponents() const
	{
		return m_comps;
	}

	void clear() { m_comps.clear(); }

private:
	std::map<std::wstring, ObjectPtr> m_comps;
};

