#pragma once


#include <Unknwn.h>
#include <vector>

#include <osg/Vec4>

#include "ObjectManager.h"

class MyVector3D
{
public:
	MyVector3D() {}
	MyVector3D(double x, double y, double z)
		: _x(x), _y(y), _z(z) {}

	MyVector3D operator+(const MyVector3D& vector3D) {
		return MyVector3D(this->_x + vector3D._x, this->_y + vector3D._y, this->_z + vector3D._z);
	}

	MyVector3D operator*(const double& coe) {
		return MyVector3D(this->_x * coe, this->_y * coe, this->_z * coe);
	}

	friend bool operator==(const MyVector3D& vec1,const MyVector3D& vec2) {
		return vec1._x == vec2._x && vec1._y == vec2._y && vec1._z == vec2._z;
	}

	double _x;
	double _y;
	double _z;
};

class MyColor
{
public:
	MyColor() {}
	MyColor(double r, double g, double b, double a)
		: _r(r), _g(g), _b(b), _a(a) {}

	double _r;
	double _g;
	double _b;
	double _a;
};

class MyPolygon
{
public:
	MyPolygon() {}

	MyVector3D m_Coord1;
	MyVector3D m_Normal1;
	MyVector3D m_Coord2;
	MyVector3D m_Normal2;
	MyVector3D m_Coord3;
	MyVector3D m_Normal3;
	MyColor m_Color;
};

class please
{
public:
   static void doit(IUnknown* iunk_state);

   //// by Xiaodong Liang March 10th
   //// get the primitives of the model
   static void doit_primitive(IUnknown* iunk_state);
   static void walkNode(IUnknown* iunk_node, osg::Vec4 color, const std::wstring& id = L"", bool bFoundFirst = false);
   static void calObjCount(IUnknown* iunk_node, bool bFoundFirst = false);
   static long _geonodecount; 
   static long _fragscount;

   static ObjectManager objectManager;
};

