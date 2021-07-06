
//------------------------------------------------------------------
// NavisWorks Sample code
//------------------------------------------------------------------

// (C) Copyright 2018 by Autodesk Inc.

// Permission to use, copy, modify, and distribute this software in
// object code form for any purpose and without fee is hereby granted,
// provided that the above copyright notice appears in all copies and
// that both that copyright notice and the limited warranty and
// restricted rights notice below appear in all supporting
// documentation.

// AUTODESK PROVIDES THIS PROGRAM "AS IS" AND WITH ALL FAULTS.
// AUTODESK SPECIFICALLY DISCLAIMS ANY IMPLIED WARRANTY OF
// MERCHANTABILITY OR FITNESS FOR A PARTICULAR USE.  AUTODESK
// DOES NOT WARRANT THAT THE OPERATION OF THE PROGRAM WILL BE
// UNINTERRUPTED OR ERROR FREE.
//------------------------------------------------------------------

#include "Raw.h"

#include <atlbase.h>
#include <atlcom.h>
#include <comdef.h>

#include <stdexcept>
#include <vector>
#include <map>
#include <xmllite.h>
#include <filesystem>

#include <boost/algorithm/string.hpp>
#include <boost/beast/core/detail/base64.hpp>
#include <boost/scope_exit.hpp>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <osg/Matrix>

#include <Geometric/Triangles.hpp>
#include <Geometric/SerializeVexel.hpp>

#import "lcodieD.dll"  rename_namespace("raw")

#pragma comment(lib, "xmllite.lib")

namespace fs = std::experimental::filesystem;
std::wstring_convert<std::codecvt_utf8<wchar_t>> u8cvt;
//////////////////////////////////////////////////////////////////////////
//using namespace System;


CComModule _Module;
long please::_geonodecount;
long please::_fragscount;
ObjectManager please::objectManager;

double g_scale = 1;

//ref class dateClass {
//public:
//	static System::DateTime stTime; 
//	static System::IO::StreamWriter^ outfile;
//
//};  


class CSeeker : public ATL::CComObjectRoot,
                public IDispatchImpl<raw::InwSeekSelection>
            
{
public:
   BEGIN_COM_MAP(CSeeker)
      COM_INTERFACE_ENTRY(raw::InwSeekSelection)
   END_COM_MAP()

   STDMETHOD(raw_SelectNode)(/*[in]*/ struct raw::InwOaNode* node,
                             /*[in]*/ struct raw::InwOaPath* path,
                             /*[in,out]*/ VARIANT_BOOL* Add,
                             /*[in,out]*/ VARIANT_BOOL* finished) 
   {
      return S_OK;
   }
                             
   CSeeker()
   {
   
   }

}; 


void please::doit(IUnknown* iunk_state) 
{
   raw::InwOpState10Ptr state(iunk_state);


   raw::InwOpSelectionPtr selection=state->ObjectFactory(raw::eObjectType_nwOpSelection);

   CComObject<CSeeker> *cseeker;

   HRESULT HR=CComObject<CSeeker>::CreateInstance(&cseeker);
   raw::InwSeekSelectionPtr seeker=cseeker->GetUnknown();//???
   state->SeekSelection(selection,seeker);
}

//// by Xiaodong Liang March 10th
//// get the primitives of the model

// callback class
class CallbackGeomClass :public ATL::CComObjectRoot,
	public IDispatchImpl<raw::InwSimplePrimitivesCB>
{
public:
	BEGIN_COM_MAP(CallbackGeomClass)
		COM_INTERFACE_ENTRY(raw::InwSimplePrimitivesCB)
	END_COM_MAP()

	MyVector3D AsVector3D(_variant_t v)
	{
		float* data = nullptr;
		SafeArrayAccessData(v.parray, (void HUGEP**)&data);
		MyVector3D pVector3D = MyVector3D(data[0], data[1], data[2]);
		::SafeArrayUnaccessData(v.parray);

		return pVector3D;
	}

	MyColor AsColor(_variant_t v)
	{
		float* data = nullptr;
		SafeArrayAccessData(v.parray, (void HUGEP**)&data);
		MyColor pColor = MyColor(data[0], data[1], data[2], data[3]);
		::SafeArrayUnaccessData(v.parray);

		return pColor;
	}
	//////////////////////////////////////////////////////////////////////////
	std::vector<MyPolygon> m_PolygonList;
	raw::InwLTransform3f3Ptr LCS2WCS;
	double zoomFactor;
	bool isReverseNormal;

	STDMETHOD(raw_Triangle)(
		/*[in]*/ struct raw::InwSimpleVertex* v1,
		/*[in]*/ struct raw::InwSimpleVertex* v2,
		/*[in]*/ struct raw::InwSimpleVertex* v3
		)
	{
		// do your work
		MyPolygon polygon;
		polygon.m_Normal1 = AsVector3D(v1->Getnormal());
		polygon.m_Normal2 = AsVector3D(v2->Getnormal());
		polygon.m_Normal3 = AsVector3D(v3->Getnormal());

		polygon.m_Coord1 = AsVector3D(v1->Getcoord());
		polygon.m_Coord1 = GetRightPoint(polygon.m_Coord1);

		polygon.m_Coord2 = AsVector3D(v2->Getcoord());
		polygon.m_Coord2 = GetRightPoint(polygon.m_Coord2);

		polygon.m_Coord3 = AsVector3D(v3->Getcoord());
		polygon.m_Coord3 = GetRightPoint(polygon.m_Coord3);

		polygon.m_Color = AsColor(v1->Getcolor());

		m_PolygonList.emplace_back(polygon);


		return S_OK;
	}

	STDMETHOD(raw_Line)(/*[in]*/ struct raw::InwSimpleVertex* v1,
		/*[in]*/ struct raw::InwSimpleVertex* v2
		)
	{


		return S_OK;
	}

	STDMETHOD(raw_Point)(/*[in]*/ struct raw::InwSimpleVertex* v1)
	{


		return S_OK;
	}

	STDMETHOD(raw_SnapPoint)(/*[in]*/ struct raw::InwSimpleVertex* v1)
	{



		return S_OK;
	}

	CallbackGeomClass()
	{

	}
private:

	MyVector3D GetRightPoint(const MyVector3D& vector3D)
	{
		osg::Matrixd mat;
		_variant_t matrix = LCS2WCS->GetMatrix();

		double* data = nullptr;
		SafeArrayAccessData(matrix.parray, (void HUGEP**) & data);
		mat = osg::Matrixd(data);
		::SafeArrayUnaccessData(matrix.parray);

		mat *= osg::Matrixd::scale(zoomFactor,zoomFactor,zoomFactor);

		osg::Vec3d ptOrigin(1, 1, 1);
		ptOrigin = ptOrigin * osg::Matrixd::scale(zoomFactor, zoomFactor, zoomFactor);

		osg::Vec3d point(vector3D._x, vector3D._y, vector3D._z);
		point = point * mat;

		return MyVector3D(point.x(), point.y(), point.z());
		//////////////////////////////////////////////////////////////////////////
		//Array array = (Array)(object)LCS2WCS.Matrix;
		//Matrix3 matrix = new Matrix3(
		//	Convert.ToDouble(array.GetValue(1)), Convert.ToDouble(array.GetValue(2)), Convert.ToDouble(array.GetValue(3)),
		//	Convert.ToDouble(array.GetValue(5)), Convert.ToDouble(array.GetValue(6)), Convert.ToDouble(array.GetValue(7)),
		//	Convert.ToDouble(array.GetValue(9)), Convert.ToDouble(array.GetValue(10)), Convert.ToDouble(array.GetValue(11))
		//);

		//Vector3D trans = new Vector3D(
		//	Convert.ToDouble(array.GetValue(13)), Convert.ToDouble(array.GetValue(14)), Convert.ToDouble(array.GetValue(15)));
		//Transform3D transform = new Transform3D(matrix, trans);
		//Matrix3 matrixinverse = transform.Linear;
		//p1 = p1.Multiply(matrixinverse);

		return vector3D;
	}
};

osg::Vec3 toVec3(const MyVector3D& vec)
{
	return osg::Vec3(vec._x, vec._y, vec._z);
}

class MySerializeContext : public FTKernel::SerializeContext
{
public:
	virtual bool isReuse(const std::string& name) const override
	{
		return false;
	}

	virtual void writeImage(osg::ref_ptr<osg::Image> image) override
	{

	}
};

// walk through the model and get the primitives
void please::walkNode(IUnknown* iunk_node, osg::Vec4 color, const std::wstring& id, bool bFoundFirst)
{
	raw::InwOaNodePtr node(iunk_node);

	// Add by lt 2021-6-10 17:43 cause :Test Properties
	auto obj = objectManager.getOrCreateComponent(id);

	std::wstring LcOaSceneBaseUserName = node->GetUserName();
	std::wstring LcOaSceneBaseClassUserName = node->GetClassUserName();
	obj->category = LcOaSceneBaseClassUserName;

	std::wstring objName = node->GetUserNameW();
	obj->addProperty(L"Name", objName);

	if (objName == L"Recreation Dynamics Ski-1")
	{
		int n = 0;
	}

	_variant_t nodeAttr;
	raw::InwNodeAttributesCollPtr nodeAttributes = node->Attributes();
	IEnumVARIANTPtr attrEnum = nodeAttributes->Get_NewEnum();
	while (attrEnum->Next(1, &nodeAttr, nullptr) == S_OK)
	{
		raw::InwOaAttributePtr attr = nodeAttr.punkVal;
		std::wstring className = attr->ClassName;

		raw::InwOaPublishAttributePtr pubAttr = nodeAttr.punkVal;
		raw::InwOaTransformPtr tranAttr = nodeAttr.punkVal;
		raw::InwOaMaterialPtr materialAttr = nodeAttr.punkVal;
		raw::InwOaNameAttributePtr nameAttr = nodeAttr.punkVal;
		raw::InwOaPropertyAttributePtr propAttr = nodeAttr.punkVal;
		raw::InwOaTextAttributePtr textAttr = nodeAttr.punkVal;
		raw::InwOaURLAttributePtr urlAttr = nodeAttr.punkVal;

		if (materialAttr)
		{
			auto diffuseColor = materialAttr->GetDiffuseColor();
			color.r() = diffuseColor->data1;
			color.g() = diffuseColor->data2;
			color.b() = diffuseColor->data3;
		}

		if (!propAttr)
			continue;

		_variant_t prop;
		raw::InwOaPropertyCollPtr properties = propAttr->Properties();
		IEnumVARIANTPtr propEnum = properties->Get_NewEnum();
		while (propEnum->Next(1, &prop, nullptr) == S_OK)
		{
			raw::InwOaPropertyPtr propTemp = prop.punkVal;

			if (!propTemp)
				continue;

			std::wstring propName = propTemp->GetUserName();
			std::wstring propValue = propTemp->Getvalue().bstrVal;

			obj->addProperty(propName, propValue);
		}
	}
	// end

	// If this is a group node then recurse into the structure
	if (node->IsGroup)
	{
		raw::InwOaGroupPtr group = (raw::InwOaGroupPtr)node;
		long subNodesCount = group->Children()->GetCount();

		//for (long subNodeIndex = 1; subNodeIndex <= subNodesCount; subNodeIndex++)
		//{
		//	if ((!bFoundFirst) && (subNodesCount > 1))
		//	{
		//		bFoundFirst = true;
		//	}
		//	raw::InwOaNodePtr newNode = group->Children()->GetItem(_variant_t(subNodeIndex));
		//	walkNode(newNode, bFoundFirst);
		//}

		// Add by lt 2021-6-9 16:09 cause :	Optimizing
		_variant_t child;
		IEnumVARIANTPtr newEnum = group->Children()->Get_NewEnum();
		while (newEnum->Next(1, &child, nullptr) == S_OK)
		{
			if ((!bFoundFirst) && (subNodesCount > 1))
			{
				bFoundFirst = true;
			}

			raw::InwOaNodePtr newNode = child.punkVal;

			auto sChildId = boost::uuids::to_wstring(boost::uuids::random_generator()());
			auto objChild = objectManager.getOrCreateComponent(sChildId);
			objChild->parentId = id;

			walkNode(newNode, color, sChildId, bFoundFirst);
		}
		// end
	}
	else if (node->IsGeometry)
	{
		long fragsCount = node->Fragments()->Count;
		please::_geonodecount += 1; // one more node

		//System::Diagnostics::Debug::WriteLine("frags count:" + fragsCount.ToString());
		//for (long fragindex = 1;fragindex <= fragsCount;fragindex++)
		//{
		//	CComObject<CallbackGeomClass>* callbkListener;
		//	HRESULT HR = CComObject<CallbackGeomClass>::CreateInstance(&callbkListener);

		//	raw::InwOaFragment3Ptr frag = node->Fragments()->GetItem(_variant_t(fragindex));
		//	frag->GenerateSimplePrimitives(
		//		raw::nwEVertexProperty::eNORMAL,
		//		callbkListener);

		//	please::_fragscount++;
		//}

		// Add by lt 2021-6-9 16:08 cause : Optimizing
		static CComObject<CallbackGeomClass>* callbkListener;
		if (!callbkListener)
		{
			CComObject<CallbackGeomClass>::CreateInstance(&callbkListener);
		}

		_variant_t fragVar;
		IEnumVARIANTPtr fragEnum = node->Fragments()->Get_NewEnum();
		while (fragEnum->Next(1, &fragVar, nullptr) == S_OK)
		{
			raw::InwOaFragment3Ptr frag = fragVar.punkVal;
			callbkListener->LCS2WCS = frag->GetLocalToWorldMatrix();
			callbkListener->zoomFactor = g_scale;
			callbkListener->m_PolygonList.resize(0);

			frag->GenerateSimplePrimitives(raw::nwEVertexProperty::eNORMAL, callbkListener);

			please::_fragscount++;

			osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array();
			osg::ref_ptr<osg::Vec3Array> normals = new osg::Vec3Array(osg::Array::BIND_PER_VERTEX);
			osg::ref_ptr<osg::UIntArray> triangleStrip = new osg::UIntArray();

			std::map<std::shared_ptr<MyVector3D>, int> mapCoordToIndex;
			for each (const auto & polygon in callbkListener->m_PolygonList)
			{
				int tempCIndex = 0;
				int tempNIndex = 0;
				auto pCoordIter = std::find_if(mapCoordToIndex.begin(), mapCoordToIndex.end(),
					[&](auto& pairItem) { return *pairItem.first == polygon.m_Coord1;});
				if (pCoordIter != mapCoordToIndex.end()
					&& toVec3(polygon.m_Normal1) == normals->at(pCoordIter->second))
				{
					triangleStrip->push_back(pCoordIter->second);
				}
				else
				{
					mapCoordToIndex[std::make_shared<MyVector3D>(polygon.m_Coord1)] = vertices->size();

					triangleStrip->push_back(vertices->size());
					vertices->push_back(toVec3(polygon.m_Coord1));
					normals->push_back(toVec3(polygon.m_Normal1));
				}

				pCoordIter = std::find_if(mapCoordToIndex.begin(), mapCoordToIndex.end(),
					[&](auto& pairItem) { return *pairItem.first == polygon.m_Coord2;});
				if (pCoordIter != mapCoordToIndex.end()
					&& toVec3(polygon.m_Normal2) == normals->at(pCoordIter->second))
				{
					triangleStrip->push_back(pCoordIter->second);
				}
				else
				{
					mapCoordToIndex[std::make_shared<MyVector3D>(polygon.m_Coord2)] = vertices->size();

					triangleStrip->push_back(vertices->size());
					vertices->push_back(toVec3(polygon.m_Coord2));
					normals->push_back(toVec3(polygon.m_Normal2));
				}

				pCoordIter = std::find_if(mapCoordToIndex.begin(), mapCoordToIndex.end(),
					[&](auto& pairItem) { return *pairItem.first == polygon.m_Coord3;});
				if (pCoordIter != mapCoordToIndex.end()
					&& toVec3(polygon.m_Normal3) == normals->at(pCoordIter->second))
				{
					triangleStrip->push_back(pCoordIter->second);
				}
				else
				{
					mapCoordToIndex[std::make_shared<MyVector3D>(polygon.m_Coord3)] = vertices->size();

					triangleStrip->push_back(vertices->size());
					vertices->push_back(toVec3(polygon.m_Coord3));
					normals->push_back(toVec3(polygon.m_Normal3));
				}
			}

			MySerializeContext context;

			try
			{
				std::vector<osg::ref_ptr<FTKernel::Vexel>> vexels;
				vexels.emplace_back(new FTKernel::Triangles(normals, vertices, triangleStrip, color));
				std::string tempData = FTKernel::serialize(vexels, context);

				obj->geometryDatas.emplace_back(std::make_shared<std::string>(tempData));
			}
			catch (...)
			{
			}
		}
		// end


		//System::DateTime nowTime = System::DateTime::Now;
		//System::TimeSpan span = nowTime.Subtract(dateClass::stTime);
		//dateClass::span = dateClass::nowTime.Subtract()
		//System::Diagnostics::Debug::WriteLine(please::_geonodecount +  "node done:" + span.TotalMilliseconds.ToString());
	}
}

void please::calObjCount(IUnknown* iunk_node, bool bFoundFirst /*= false*/)
{
	raw::InwOaNodePtr node(iunk_node);

	// If this is a group node then recurse into the structure
	if (node->IsGroup)
	{
		raw::InwOaGroupPtr group = (raw::InwOaGroupPtr)node;
		long subNodesCount = group->Children()->GetCount();

		_variant_t child;
		IEnumVARIANTPtr newEnum = group->Children()->Get_NewEnum();
		while (newEnum->Next(1, &child, nullptr) == S_OK)
		{
			if ((!bFoundFirst) && (subNodesCount > 1))
			{
				bFoundFirst = true;
			}

			raw::InwOaNodePtr newNode = child.punkVal;
			calObjCount(newNode, bFoundFirst);
		}
	}
	else if (node->IsGeometry)
	{
		++please::_geonodecount;
		please::_fragscount += node->Fragments()->Count;
	}
}


inline void VerifyHR(HRESULT hr)
{
	if (FAILED(hr))
		_com_issue_error(hr);
}

struct ItemNode
{
	ItemNode(const std::wstring& id
		, const std::wstring& type
		, const std::wstring& name)
		: m_ID(id), m_Type(type), m_Name(name)
	{
	}
	std::wstring m_ID;
	std::wstring m_Name;
	std::wstring m_Type;
	std::shared_ptr<ItemNode> m_Parent;
	std::vector<std::shared_ptr<ItemNode> > m_Childs;
};

void testWriteItems(std::shared_ptr<ItemNode> itemNode, CComPtr<IXmlWriter> writer)
{
	VerifyHR(writer->WriteStartElement(NULL, L"item", NULL));
	VerifyHR(writer->WriteWhitespace(L"\n\t"));

	VerifyHR(writer->WriteElementString(NULL, L"itemId", NULL, itemNode->m_ID.c_str()));
	VerifyHR(writer->WriteWhitespace(L"\n\t"));

	VerifyHR(writer->WriteElementString(NULL, L"category", NULL, itemNode->m_Type.c_str()));

	if (itemNode->m_Parent.get() != NULL
		&& !itemNode->m_Parent->m_ID.empty())
	{
		VerifyHR(writer->WriteWhitespace(L"\n\t"));
		VerifyHR(writer->WriteElementString(NULL, L"parentItemId", NULL, itemNode->m_Parent->m_ID.c_str()));
	}
	VerifyHR(writer->WriteWhitespace(L"\n"));

	VerifyHR(writer->WriteEndElement());
	VerifyHR(writer->WriteWhitespace(L"\n"));

	for (auto& child : itemNode->m_Childs)
	{
		testWriteItems(child, writer);
	}
}

void writeXml(ObjectManager& compMngr, const fs::path& xmlFilename)
{
	IStreamPtr outFileStream;
	VerifyHR(SHCreateStreamOnFile(xmlFilename.wstring().c_str(), STGM_CREATE | STGM_WRITE, &outFileStream));

	CComPtr<IXmlWriter> writer;
	VerifyHR(CreateXmlWriter(__uuidof(IXmlWriter), (void**)&writer, NULL));
	VerifyHR(writer->SetOutput(outFileStream));
	VerifyHR(writer->SetProperty(XmlWriterProperty_Indent, FALSE));
	VerifyHR(writer->WriteStartDocument(XmlStandalone_Omit));
	VerifyHR(writer->WriteWhitespace(L"\n"));

	VerifyHR(writer->WriteStartElement(NULL, L"objects", NULL));
	VerifyHR(writer->WriteAttributeString(NULL, L"version", NULL, L"4.0.0"));
	VerifyHR(writer->WriteWhitespace(L"\n"));


	//Item顺序必须是按照根到子的顺序写出
	std::vector<std::shared_ptr<ItemNode> > roots;
	std::map<std::wstring, std::shared_ptr<ItemNode> > items;

	auto& comps = compMngr.getComponents();
	for (auto& compItem : comps)
	{
		ObjectPtr comp = compItem.second;
		std::shared_ptr<ItemNode> itemNode;
		if (items.find(comp->id) == items.end())
		{
			itemNode.reset(new ItemNode(comp->id, comp->category, L""));
			items[comp->id] = itemNode;
		}
		else
		{
			itemNode = items[comp->id];
			itemNode->m_ID = comp->id;
			itemNode->m_Name = L"";
			itemNode->m_Type = comp->category;
		}

		//构建父子关系
		if (!comp->parentId.empty())
		{
			std::shared_ptr<ItemNode> parentItem;
			std::map<std::wstring, std::shared_ptr<ItemNode> >::iterator iFind;
			iFind = items.find(comp->parentId);
			if (iFind != items.end())
			{
				parentItem = iFind->second;
			}
			else
			{
				parentItem.reset(new ItemNode(comp->parentId, L"", L""));
				items[comp->parentId] = parentItem;
			}

			parentItem->m_Childs.push_back(itemNode);
			itemNode->m_Parent = parentItem;
		}
		else
		{
			roots.push_back(itemNode);
		}
	}

	//按照先后顺序写出Item
	for (auto& root : roots)
	{
		testWriteItems(root, writer);
	}


	for (auto& compItem : comps)
	{
		ObjectPtr comp = compItem.second;
		if (comp->geometryDatas.empty())
			continue;

		VerifyHR(writer->WriteStartElement(NULL, L"geometry", NULL));
		VerifyHR(writer->WriteWhitespace(L"\n\t"));

		VerifyHR(writer->WriteElementString(NULL, L"itemId", NULL, compItem.first.c_str()));
		VerifyHR(writer->WriteWhitespace(L"\n\t"));

		std::string allData;
		for (auto& data : comp->geometryDatas)
		{
			allData += *data;
		}

		std::string base64Data = boost::beast::detail::base64_encode(allData);
		std::wstring str = u8cvt.from_bytes(base64Data);
		VerifyHR(writer->WriteElementString(NULL, L"data", NULL, str.c_str()));
		VerifyHR(writer->WriteWhitespace(L"\n"));

		VerifyHR(writer->WriteEndElement());
		VerifyHR(writer->WriteWhitespace(L"\n"));
	}

	for (auto& compItem : comps)
	{
		ObjectPtr comp = compItem.second;
		if (comp->properties.empty())
			continue;

		for (auto& prop : comp->properties)
		{
			VerifyHR(writer->WriteStartElement(NULL, L"property", NULL));
			VerifyHR(writer->WriteWhitespace(L"\n\t"));

			VerifyHR(writer->WriteElementString(NULL, L"itemId", NULL, compItem.first.c_str()));
			VerifyHR(writer->WriteWhitespace(L"\n\t"));

			VerifyHR(writer->WriteElementString(NULL, L"name", NULL, prop.second->name.c_str()));
			VerifyHR(writer->WriteWhitespace(L"\n\t"));

			VerifyHR(writer->WriteElementString(NULL, L"value", NULL, prop.second->value.c_str()));
			VerifyHR(writer->WriteWhitespace(L"\n"));

			VerifyHR(writer->WriteEndElement());
			VerifyHR(writer->WriteWhitespace(L"\n"));
		}
	}

	VerifyHR(writer->WriteEndElement());
	VerifyHR(writer->WriteEndDocument());
	VerifyHR(writer->Flush());
}

// do primitive
void please::doit_primitive(IUnknown* iunk_state) 
{
	please::_geonodecount = 0;
	please::_fragscount = 0;

    raw::InwOpState10Ptr state(iunk_state);
    raw::InwOaPartition3Ptr   oP = state->CurrentPartition;
	
	auto unitType = oP->GetLinearUnits();
	if (unitType == raw::eLinearUnits_METRES)
	{
		g_scale *= 1000;
	}
   
	calObjCount(oP, false);
	//dateClass::stTime = DateTime::Now;
	//dateClass::outfile = gcnew System::IO::StreamWriter("c:\\temp\\dump.rtf");

	_geonodecount = 0;
	_fragscount = 0;
	objectManager.clear();

	auto rootId = boost::uuids::to_wstring(boost::uuids::random_generator()());
	osg::Vec4 defaultColor(0.4, 0.4, 0.4, 1.0);

	walkNode(oP, defaultColor, rootId, false);

	writeXml(objectManager, LR"~(e:\WorkSln\IntelliTools2.0\trunk\src\bin\Config\Samples\Samples\Navisworks\AIP_Test\model.xml)~");

	//dateClass::outfile->Close();

	//System::DateTime nowTime = System::DateTime::Now;
	//System::TimeSpan span = nowTime.Subtract(dateClass::stTime);
	////dateClass::span = dateClass::nowTime.Subtract()
	//System::Diagnostics::Debug::WriteLine(span.TotalMilliseconds.ToString()); 
	//System::Windows::Forms::MessageBox::Show(span.TotalMilliseconds.ToString() + 
	//	" ms, Geometry Node: "+ please::_geonodecount +
	//    "Fragments: " + please::_fragscount);
}

