// ***************************************************************************
//
//   Generated automatically by genwrapper.
//   Please DO NOT EDIT this file!
//
// ***************************************************************************

#include <osgIntrospection/ReflectionMacros>
#include <osgIntrospection/TypedMethodInfo>
#include <osgIntrospection/StaticMethodInfo>
#include <osgIntrospection/Attributes>

#include <osg/CopyOp>
#include <osg/Matrixd>
#include <osg/Object>
#include <osg/Plane>
#include <osg/State>
#include <osg/StateAttribute>
#include <osg/TexGen>

// Must undefine IN and OUT macros defined in Windows headers
#ifdef IN
#undef IN
#endif
#ifdef OUT
#undef OUT
#endif

BEGIN_ENUM_REFLECTOR(osg::TexGen::Mode)
	I_DeclaringFile("osg/TexGen");
	I_EnumLabel(osg::TexGen::OBJECT_LINEAR);
	I_EnumLabel(osg::TexGen::EYE_LINEAR);
	I_EnumLabel(osg::TexGen::SPHERE_MAP);
	I_EnumLabel(osg::TexGen::NORMAL_MAP);
	I_EnumLabel(osg::TexGen::REFLECTION_MAP);
END_REFLECTOR

BEGIN_ENUM_REFLECTOR(osg::TexGen::Coord)
	I_DeclaringFile("osg/TexGen");
	I_EnumLabel(osg::TexGen::S);
	I_EnumLabel(osg::TexGen::T);
	I_EnumLabel(osg::TexGen::R);
	I_EnumLabel(osg::TexGen::Q);
END_REFLECTOR

BEGIN_OBJECT_REFLECTOR(osg::TexGen)
	I_DeclaringFile("osg/TexGen");
	I_BaseType(osg::StateAttribute);
	I_Constructor0(____TexGen,
	               "",
	               "");
	I_ConstructorWithDefaults2(IN, const osg::TexGen &, texgen, , IN, const osg::CopyOp &, copyop, osg::CopyOp::SHALLOW_COPY,
	                           ____TexGen__C5_TexGen_R1__C5_CopyOp_R1,
	                           "Copy constructor using CopyOp to manage deep vs shallow copy. ",
	                           "");
	I_Method0(osg::Object *, cloneType,
	          Properties::VIRTUAL,
	          __osg_Object_P1__cloneType,
	          "Clone the type of an attribute, with Object* return type. ",
	          "Must be defined by derived classes. ");
	I_Method1(osg::Object *, clone, IN, const osg::CopyOp &, x,
	          Properties::VIRTUAL,
	          __osg_Object_P1__clone__C5_osg_CopyOp_R1,
	          "Clone an attribute, with Object* return type. ",
	          "Must be defined by derived classes. ");
	I_Method1(bool, isSameKindAs, IN, const osg::Object *, obj,
	          Properties::VIRTUAL,
	          __bool__isSameKindAs__C5_osg_Object_P1,
	          "Return true if this and obj are of the same kind of object. ",
	          "");
	I_Method0(const char *, libraryName,
	          Properties::VIRTUAL,
	          __C5_char_P1__libraryName,
	          "Return the name of the attribute's library. ",
	          "");
	I_Method0(const char *, className,
	          Properties::VIRTUAL,
	          __C5_char_P1__className,
	          "Return the name of the attribute's class type. ",
	          "");
	I_Method0(osg::StateAttribute::Type, getType,
	          Properties::VIRTUAL,
	          __Type__getType,
	          "Return the Type identifier of the attribute's class type. ",
	          "");
	I_Method0(bool, isTextureAttribute,
	          Properties::VIRTUAL,
	          __bool__isTextureAttribute,
	          "Return true if StateAttribute is a type which controls texturing and needs to be issued w.r.t to specific texture unit. ",
	          "");
	I_Method1(int, compare, IN, const osg::StateAttribute &, sa,
	          Properties::VIRTUAL,
	          __int__compare__C5_StateAttribute_R1,
	          "Return -1 if *this < *rhs, 0 if *this==*rhs, 1 if *this>*rhs. ",
	          "");
	I_Method1(bool, getModeUsage, IN, osg::StateAttribute::ModeUsage &, x,
	          Properties::VIRTUAL,
	          __bool__getModeUsage__StateAttribute_ModeUsage_R1,
	          "Return the modes associated with this StateAttribute. ",
	          "");
	I_Method1(void, apply, IN, osg::State &, x,
	          Properties::VIRTUAL,
	          __void__apply__State_R1,
	          "apply the OpenGL state attributes. ",
	          "The render info for the current OpenGL context is passed in to allow the StateAttribute to obtain details on the the current context and state. ");
	I_Method1(void, setMode, IN, osg::TexGen::Mode, mode,
	          Properties::NON_VIRTUAL,
	          __void__setMode__Mode,
	          "",
	          "");
	I_Method0(osg::TexGen::Mode, getMode,
	          Properties::NON_VIRTUAL,
	          __Mode__getMode,
	          "",
	          "");
	I_Method2(void, setPlane, IN, osg::TexGen::Coord, which, IN, const osg::Plane &, plane,
	          Properties::NON_VIRTUAL,
	          __void__setPlane__Coord__C5_Plane_R1,
	          "",
	          "");
	I_Method1(osg::Plane &, getPlane, IN, osg::TexGen::Coord, which,
	          Properties::NON_VIRTUAL,
	          __Plane_R1__getPlane__Coord,
	          "",
	          "");
	I_Method1(const osg::Plane &, getPlane, IN, osg::TexGen::Coord, which,
	          Properties::NON_VIRTUAL,
	          __C5_Plane_R1__getPlane__Coord,
	          "",
	          "");
	I_Method1(void, setPlanesFromMatrix, IN, const osg::Matrixd &, matrix,
	          Properties::NON_VIRTUAL,
	          __void__setPlanesFromMatrix__C5_Matrixd_R1,
	          "Set the tex gen planes from specified matrix. ",
	          "Typical usage would be to pass in a projection matrix to set up projective texturing. ");
	I_SimpleProperty(osg::TexGen::Mode, Mode, 
	                 __Mode__getMode, 
	                 __void__setMode__Mode);
	I_IndexedProperty(const osg::Plane &, Plane, 
	                  __C5_Plane_R1__getPlane__Coord, 
	                  __void__setPlane__Coord__C5_Plane_R1, 
	                  0);
	I_SimpleProperty(const osg::Matrixd &, PlanesFromMatrix, 
	                 0, 
	                 __void__setPlanesFromMatrix__C5_Matrixd_R1);
	I_SimpleProperty(osg::StateAttribute::Type, Type, 
	                 __Type__getType, 
	                 0);
END_REFLECTOR
