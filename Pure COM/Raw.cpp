
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
#include <stdexcept>
#include <vector>

#import "lcodieD.dll"  rename_namespace("raw")

//using namespace System;


CComModule _Module;
long please::_geonodecount;
long please::_fragscount;

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
		double xTrans = LCS2WCS->GetTranslation()->data1;
		double yTrans = LCS2WCS->GetTranslation()->data2;
		double zTrans = LCS2WCS->GetTranslation()->data3;
		MyVector3D shift(xTrans, yTrans, zTrans);

		MyPolygon polygon;
		polygon.m_Normal1 = AsVector3D(v1->Getnormal());
		polygon.m_Normal2 = AsVector3D(v2->Getnormal());
		polygon.m_Normal3 = AsVector3D(v3->Getnormal());

		polygon.m_Coord1 = AsVector3D(v1->Getcoord());
		polygon.m_Coord1 = GetRightPoint(polygon.m_Coord1);
		polygon.m_Coord1 = TransformPoint((polygon.m_Coord1 + shift) * zoomFactor);

		polygon.m_Coord2 = AsVector3D(v2->Getcoord());
		polygon.m_Coord2 = GetRightPoint(polygon.m_Coord2);
		polygon.m_Coord2 = TransformPoint((polygon.m_Coord2 + shift) * zoomFactor);

		polygon.m_Coord3 = AsVector3D(v3->Getcoord());
		polygon.m_Coord3 = GetRightPoint(polygon.m_Coord3);
		polygon.m_Coord3 = TransformPoint((polygon.m_Coord3 + shift) * zoomFactor);

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
		return vector3D;
	}

	MyVector3D TransformPoint(const MyVector3D& vector3D)
	{
		return vector3D;
	}


};

// walk through the model and get the primitives
void please::walkNode(IUnknown* iunk_node, bool bFoundFirst)
{
	raw::InwOaNodePtr node(iunk_node);

	// Add by lt 2021-6-10 17:43 cause :Test Properties
	std::wstring LcOaSceneBaseUserName = node->GetUserName();
	std::wstring LcOaSceneBaseClassUserName = node->GetClassUserName();

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


		//_variant_t prop;
		//raw::InwOaPropertyCollPtr properties = propAttr->Properties();
		//IEnumVARIANTPtr propEnum = nodeAttributes->Get_NewEnum();
		//while (attrEnum->Next(1, &prop, nullptr) == S_OK)
		//{
		//	raw::InwOaPropertyPtr propTemp = prop.punkVal;
		//	std::wstring propName = propTemp->Getname();
		//	std::wstring propValue = propTemp->Getvalue().bstrVal;
		//}
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
			walkNode(newNode, bFoundFirst);
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
		std::vector<MyPolygon> polygons;

		_variant_t fragVar;
		IEnumVARIANTPtr fragEnum = node->Fragments()->Get_NewEnum();
		while (fragEnum->Next(1, &fragVar, nullptr) == S_OK)
		{
			CComObject<CallbackGeomClass>* callbkListener;
			HRESULT HR = CComObject<CallbackGeomClass>::CreateInstance(&callbkListener);

			raw::InwOaFragment3Ptr frag = fragVar.punkVal;
			callbkListener->LCS2WCS = frag->GetLocalToWorldMatrix();
			callbkListener->zoomFactor = 1;

			frag->GenerateSimplePrimitives(raw::nwEVertexProperty::eNORMAL, callbkListener);
			polygons.insert(polygons.end(),callbkListener->m_PolygonList.begin(),callbkListener->m_PolygonList.end());

			please::_fragscount++;
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

// do primitive
void 
please::doit_primitive(IUnknown* iunk_state) 
{
	please::_geonodecount = 0;
	please::_fragscount = 0;

    raw::InwOpState10Ptr state(iunk_state);
    raw::InwOaPartitionPtr   oP = state->CurrentPartition;
   
	calObjCount(oP, false);
	//dateClass::stTime = DateTime::Now;
	//dateClass::outfile = gcnew System::IO::StreamWriter("c:\\temp\\dump.rtf");

	_geonodecount = 0;
	_fragscount = 0;

    walkNode(oP, false);  

	//dateClass::outfile->Close();

	//System::DateTime nowTime = System::DateTime::Now;
	//System::TimeSpan span = nowTime.Subtract(dateClass::stTime);
	////dateClass::span = dateClass::nowTime.Subtract()
	//System::Diagnostics::Debug::WriteLine(span.TotalMilliseconds.ToString()); 
	//System::Windows::Forms::MessageBox::Show(span.TotalMilliseconds.ToString() + 
	//	" ms, Geometry Node: "+ please::_geonodecount +
	//    "Fragments: " + please::_fragscount);
}

