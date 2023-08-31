//*****************************************************************************
// $Archive: /ClearPath SC/User Driver/inc/pubSysCls.h $
// $Date: 01/19/2017 17:39 $
// $Workfile: pubSysCls.h $
//
// DESCRIPTION:
/**
	\file	
	sFoundation 2.0 Software Framework Class Library.

	sFoundation 2.0 software framework for host programming a system
	of ClearPath-SC nodes. This framework implements a rich class library
	for interacting with the nodes via C++.ZSFzsdfzdzd
**/
// PRINCIPLE AUTHORS:
//		DWS
//
// CREATION DATE:
//		2015-12-08 12:36:45
// 
// COPYRIGHT NOTICE:
//		(C)Copyright 2015-2016 Teknic, Inc.  All rights reserved.
//
//		This copyright notice must be reproduced in any copy, modification, 
//		or portion thereof merged into another program. A copy of the 
//		copyright notice must be included in the object library of a user
//		program.
//																			  *
//*****************************************************************************
#ifndef __PUBSYSCLS_H__
#define __PUBSYSCLS_H__


//*****************************************************************************
// NAME																          *
// 	pubSysCls.h headers
//
#include "tekTypes.h"
#include "pubCoreRegs.h"
#include "pubPwrReg.h"
#include "pubNetAPI.h"

// Use C++ lib strings
#include <string>
#include <vector>

//																			  *
//*****************************************************************************




//*****************************************************************************
// NAME																	      *
// 	pubSysCls.h forward references
													/** \cond INTERNAL_DOC **/	
namespace sFnd 
{
	class INode;
	class IPort;
	class IAttnPort;
	/**
		Function template for the attention callback function.
	**/
	typedef void (nodeCallback *mnAttnCallback)(const mnAttnReqReg &detected);
}
//													/** \endcond **/
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	pubSysCls.h constants
//
//
//																			  *
//*****************************************************************************

//*****************************************************************************

/**
	Teknic S-Foundation namespace.
**/
namespace sFnd 
{
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wreorder"
#endif

//*****************************************************************************
// NAME																	      *
// 	IObj Class
/**
	\brief Feature Availability Interface. 

	This is the base object for all feature objects. It provides the 
	function for determining whether a "feature object" is supported.
**/
class MN_EXPORT IObj
{
													/** \cond INTERNAL_DOC **/	
	friend class CPMinfo;
													/** \endcond **/
public:
	/**
		\brief Query if this feature is supported in this node.

		\return Returns true if this object is supported by this node.
		
		A programmatic way of determining if this node supports this feature.
		Some features are not available on certain types of nodes.
	**/
	virtual bool Supported();

													/** \cond INTERNAL_DOC **/	
private:
	// Flag showing support
	bool m_supported;

protected:
	IObj();
	IObj(bool support);

protected:
	/// Sets the supported state for this object
	void Supported(bool newState);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IObjWithNode Class
/**
	\brief Feature Availability And Root Node Access Interface. 

	This is the base object for all Node Level feature Objects, and provides a
	common function each feature to have a back link to the owner node. 
**/
class MN_EXPORT IObjWithNode : public IObj
{
public:
	/**
		\brief Reference to our node object.

		\return Reference to our node object.

		This member function is useful when you want to return the root of your
		node when you are using a feature object. For example, your are 
		interacting with a status object IStatus and you wish to disable the
		node.

		\CODE_SAMPLE_HDR
		// Access another feature from another feature
		myStatusObj.Node().EnableReq(false));
		\endcode
	**/
	INode &Node() { return *m_pNode; }
protected:
													/** \cond INTERNAL_DOC **/
	INode *m_pNode;
	IObjWithNode(INode &ourNode); 
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IObjWithPort Class
/**
	\brief Feature Availability And Port Access Interface.

	This is the base object for all Port Level feature Objects.
**/
class MN_EXPORT IObjWithPort : public IObj
{
protected:
													/** \cond INTERNAL_DOC **/
	/// Pointer to our IPort object
	IPort *m_pPort;
	/// Construction with tie-in to our port
	IObjWithPort(IPort &ourPort);
													/** \endcond **/
};
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME																	      *
// 	ValueBase class
/**

\brief Node Parameter Container Base Class. 

This class defines the base parameter access 
object with functions to determine the existence, volatility, and refresh parameters from
a node. 

This object provides your application with:
- A test for parameter existence for your node
- A test for volatile type parameters
- A Refresh control for volatile parameters such as position
**/
class MN_EXPORT ValueBase : public IObjWithNode
{
													/** \cond INTERNAL_DOC **/
	friend class ValueString;
	friend class ValueDouble;
	friend class ValueSigned;
	friend class ValueUnsigned;
	friend class ValueAlert;
	friend class ValueAppConfigReg;
	friend class ValueHwConfigReg;
	friend class ValuePowerReg;
	friend class ValueOutReg;
	friend class ValueStatus;
	friend class CPMstring;
	friend class CPMstatus;
	friend class CPMstatusAdv;
	friend class CPMouts;
	friend class CPMmotion;
private:
	double m_scaleToUser;
	nodeparam m_paramNum;
	bool m_valid;
	bool m_exists;
	bool m_isVolatile;
	bool m_refreshOnAccess;
													/** \endcond **/
public:
	/**
		\brief Update our local copy from the node

		Use this function to update the local cached copy of the node's
		parameter. For most static type parameters, there is little need to
		call this member function as the first access will perform this
		operation automatically.

		For dynamic parameters, such as position, status and velocity, use
		this function to obtain a fresh copy from the node.

		\see AutoRefresh(bool) for changing the behavior to refresh on each
		access.
	**/
	virtual void Refresh() = 0;

	/**
		\brief Determine the parameter's existence.

		Determine the parameter's existence.

		This is a helpful function when a mixture of node types may be in
		a system an some models may not support certain parameters.

		\return True if the underlying parameter exists for this node.

		
	**/
	bool Exists() { return m_exists; }

	/**
		\brief Adjust the auto-refresh setting

		Quite often a application needs to get a snapshot of a parameter for
		use during a loop through the application logic. If you don't care about
		"sample-time" consistency, you can save calling the \a Refresh
		function before each access by setting this to true.

		The default is false for this setting. This will cause one refresh
		on the first access.

		\note For performance reasons, don't set AutoRefresh on static
		type parameters, such as Acceleration Limits. Setting this true will
		cause network traffic on each inquiry to the value.

		\param[in] newState Set true to force a refresh on each value access.
		This should only be used for dynamic parameters such as position.
	**/
	void AutoRefresh(bool newState) { m_refreshOnAccess = newState; }

	/**
		\brief Return the auto-refresh state

		This feature controls when queries from a node occur. If true is returned,
		each inquiry via the Value or type conversion functions will
		result in network traffic to query the value from the node.

		If false is returned, only the first inquiry will transfer data from the
		node.

		\return Current auto-refresh state.

		
	**/
	bool AutoRefresh() { return m_refreshOnAccess; }

	/**
	\brief Determine if this parameter is a dynamic parameter.

	Determine if this parameter is a dynamic parameter, such as status.

	\return True if the value of this parameter can change after refresh.
	**/
	bool IsVolatile() { return m_isVolatile; }
													/** \cond INTERNAL_DOC **/
protected:
	/// Set Valid state
	void Valid(bool newState);
	/// Set/Get parameter number
	void ParamNum(nodeparam newParamNum);
	nodeparam ParamNum() { return m_paramNum; }
	/// Set value scale constant
	void Scale(double newScale);
	/// Construction
	ValueBase(INode &node, nodeparam pNum);
	/// Existence flag setup
	void Exists(bool newState);
	/// Set the volatile state
	void IsVolatile(bool newState) {m_isVolatile = newState;}
public:
	virtual ~ValueBase() {};
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueDouble class
/**
	\brief This class object implements access to floating point parameters from
	a node. 
	This class object implements access to floating point parameters within the node.

	For most applications the following code sequence samples illustrate
	the commonly used functions.

	\CODE_SAMPLE_HDR
	// Setting a value
	myNode.someFeature.someParameter = 12345;

	// Getting a value
	double myVal = myNode.someFeature.someParameter;

	// Using a parameter in a calculation
	double myCalVal = myNode.someFeature.someParameter.Value() + 1000;

	// Making a copy of a value. This copy will run independently of the
	// node's reference. For example, myCopy.Refresh() will only
	// update myCopy's internal value.
	ValueDouble myCopy = myNode.someFeature.someParameter;

	// Making a short-cut reference. This sometimes can reduce the typing
	// requirements for using a parameter. In this example, myRef for
	// all intents and purposes is the same as myNode.aFeature.aParameter.
	ValueDouble &myRef = myNode.someFeature.someParameter;
	myRef.Refresh();
	// is the same as
	myNode.someFeature.someParameter.Refresh();
	\endcode

	\see [Value Parameter Overview](@ref ValueSect) for more information on ValueXXXX Classes
**/
class MN_EXPORT ValueDouble : public ValueBase
{
													/** \cond INTERNAL_DOC **/
	friend class IInfo;
	friend class IHoming;
	friend class ILimits;
	friend class ILimitsAdv;
	friend class IMotion;
	friend class IMotionAdv;
	friend class IStatusAdv;
	friend class CPMnode;
	friend class IStatus;
	friend class ISetup;
	friend class ISetupEx;

protected:
	// Source of parameter value
	enum _srcs {
		VIA_PARAM
	};
	double m_lastValue;
	double m_currentValue;
	enum _srcs m_src;
													/** \endcond **/
public:
	/*
		\brief Query the node and obtain a new copy of the parameter.

		Query the node and obtain a new copy of the parameter.


		\CODE_SAMPLE_HDR
		//Refresh a volatile parameter before reading
		mynode.someFeature.someParameter.Refresh();
		\endcode
	**/
	void Refresh();

	/**
		\brief Set new double value using the assignment operator.

		Set new double value using the assignment operator.

		\param[in] newValue New value to send to the node.

		\CODE_SAMPLE_HDR
		myNode.someFeature.someParameter = 1000;
		\endcode

		\see sFnd::ValueDouble::Value For explicit function way of setting
		a parameter as well as it's power-on default value.
	**/
	double operator=(double newValue);

	/**
		\brief Get the current value and ensure it is represented as a double.

		Get the current value and ensure it is represented as a double.

		\CODE_SAMPLE_HDR
		// Implicit conversion
		double myDouble = myNode.someFeature.someParameter;
		
		// Explicit conversion
		printf("%f", double(myNode.someFeature.someParameter));
		\endcode
	**/
	operator double();

	/**
		\brief Get the current value and ensure it is represented as a signed integer.

		Get the current value and ensure it is represented as a signed integer.

		\note If the parameter is larger then +/- 31-bits the value will be truncated.
		
		\CODE_SAMPLE_HDR
		// Implicit conversion
		int32_t myInt = myNode.someFeature.someParameter;
		
		// Explicit conversion
		printf("%d", int32_t(myNode.someFeature.someParameter));
		\endcode
	**/
	operator int32_t();

	/**
		\brief Get the current value and ensure it is an unsigned integer.

		Get the current value and ensure it is an unsigned integer.

		\note If the parameter is larger then 32-bits the value will be truncated.

		\CODE_SAMPLE_HDR
		// Implicit conversion
		uint32_t myInt = myNode.someFeature.someParameter;
		
		// Explicit conversion
		printf("%u", uint32_t(myNode.someFeature.someParameter));
		\endcode
	**/
	operator uint32_t();

	/**
		\brief Change our run-time parameter and optionally update the non-volatile
		default power-up value.

		Change our run-time parameter and optionally update the non-volatile
		default power-up value.

		\param[in] newValue New value to send to the node.
		\param[in] makeNonVolatile [Optionally update the non-volatile power
		on default as well]

		\CODE_SAMPLE_HDR
		// Set the run-time value only
		myNode.someFeature.someParameter.Value(1000);
		// Set the run-time value and non-volatile power-on default
		myNode.someFeature.someParameter.Value(1000, true);
		\endcode

		\note Care should be taken to not excessively write the non-volatile
		power-up default parameter as the memory has a write count limitation.

		\see #sFnd::ValueDouble::operator= For a simpler syntax for setting
		a parameter.
	**/
	void Value(int32_t newValue, bool makeNonVolatile = false);

	/**
		\brief Change our run-time parameter and optionally update the non-volatile
		default power-up value.

		Change our run-time parameter and optionally update the non-volatile
		default power-up value.

		\param[in] newValue New value to send to the node.
		\param[in] makeNonVolatile [Optionally update the non-volatile power
		on default as well]

		\CODE_SAMPLE_HDR
		// Set the run-time value only
		myNode.someFeature.someParameter.Value(1000.5);
		// Set the run-time value and non-volatile power-on default
		myNode.someFeature.someParameter.Value(1000.5, true);
		\endcode

		\note Care should be taken to not excessively write the non-volatile
		power-up default parameter as the memory has a write count limitation.

		\see #sFnd::ValueDouble::operator= For simplified syntax to write
		the run-time value.
		**/
	void Value(double newValue, bool makeNonVolatile=false);

	/**
		\brief Return the parameter's current value as a double.

		Return the parameter's current value as a double.

		\param[in] getNonVolatile If true, the power-on default non-volatile
		value is returned.

		\CODE_SAMPLE_HDR
		// Get and print the run-time value
		cout << myNode.someFeature.someParameter.Value() << endl;
		// Get and print the power-on default non-volatile value
		cout << myNode.someFeature.someParameter.Value(true) << endl;
		\endcode
	**/
	double Value(bool getNonVolatile = false);

	/** \cond INTERNAL_DOC **/
	// - - - - - - - - - - - - - - - - - - -
	// Common overrides for signed integers
	// - - - - - - - - - - - - - - - - - - -
	double operator|(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue) | arg);
	}
	double operator&(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue) & arg);
	}
	double operator^(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue) ^ arg);
	}
	double operator+(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue + arg));
	}
	double operator-(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue - arg));
	}
	double operator*(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue * arg));
	}
	double operator/(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue / arg));
	}
	double operator|=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue) | arg);
	}
	double operator&=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue) & arg);
	}
	double operator^=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue) ^ arg);
	}
	double operator+=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue + arg));
	}
	double operator-=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue - arg));
	}
	double operator*=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue * arg));
	}
	double operator/=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue / arg));
	}

	// Common overrides for unsigned integers

	double operator|(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue) | arg);
	}
	double operator&(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue) & arg);
	}
	double operator^(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue) ^ arg);
	}
	double operator+(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	double operator-(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	double operator*(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	double operator/(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}
	double operator|=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue) | arg);
	}
	double operator&=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue) & arg);
	}
	double operator^=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue) ^ arg);
	}
	double operator+=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	double operator-=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	double operator*=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	double operator/=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}

	// Common overrides for doubles
	double operator+(double arg) {
		return(m_currentValue + arg);
	}
	double operator-(double arg) {
		return(m_currentValue - arg);
	}
	double operator*(double arg) {
		return(m_currentValue * arg);
	}
	double operator/(double arg) {
		return(m_currentValue / arg);
	}
	double operator+=(double arg) {
		return(m_currentValue + arg);
	}
	double operator-=(double arg) {
		return(m_currentValue - arg);
	}
	double operator*=(double arg) {
		return(m_currentValue * arg);
	}
	double operator/=(double arg) {
		return(m_currentValue / arg);
	}
	/** \endcond **/
protected:
													/** \cond INTERNAL_DOC **/
	// Construction to wire to our node object
	ValueDouble(INode &node, nodeparam pNum, enum _srcs src = VIA_PARAM);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueSigned class
/**
	\brief Signed 32-bit Integer Parameter Object.

	This class object implements access to integer parameters within the node.

	For most applications the following code sequence samples illustrate
	the commonly used functions.

	\CODE_SAMPLE_HDR
	// Setting a value
	myNode.someFeature.someParameter = 12345;

	// Getting a value
	int32_t myVal = myNode.someFeature.someParameter;

	// Using a parameter in a calculation
	int32_t myCalVal = myNode.someFeature.someParameter.Value() + 1000;

	// Making a copy of a value. This copy will run independently of the
	// node's reference. For example, myCopy.Refresh() will only
	// update myCopy's internal value.
	ValueSigned myCopy = myNode.someFeature.someParameter;

	// Making a short-cut reference. This sometimes can reduce the typing
	// requirements for using a parameter. In this example, aRef for
	// all intents and purposes is the same as myNode.aFeature.aParameter.
	ValueSigned &myRef = myNode.someFeature.someParameter;
	myRef.Refresh();
	// is the same as
	myNode.someFeature.someParameter.Refresh();
	\endcode

	\see [Value Parameter Overview](@ref ValueSect) for more information on ValueXXXX classes.
**/
class MN_EXPORT ValueSigned : public ValueBase
{
	/** \cond INTERNAL_DOC **/
	friend class IInfo;
	friend class IHoming;
	friend class ILimits;
	friend class IMotion;
	friend class IMotionAdv;
	friend class IStatusAdv;

private:
	int32_t m_lastValue;
	int32_t m_currentValue;
	/** \endcond **/
public:
	/**
		\brief Query the node and obtain a new copy of the parameter.

		Query the node and obtain a new copy of the parameter.


		\CODE_SAMPLE_HDR
		//Refresh a volatile parameter before reading
		mynode.someFeature.someParameter.Refresh();
		\endcode
	**/
	void Refresh();

	/**
		\brief Set new signed value using the assignment operator.

		Set new signed value using the assignment operator.

		This is the most common way to change an integer based parameter.

		\CODE_SAMPLE_HDR
		myNode.someFeature.someParameter = 1000;
		\endcode

		\see sFnd::ValueSigned::Value For an explicit way of setting
		a parameter as well as it's power-on default value.
	**/
	int32_t operator=(int32_t newValue);

	/**
		\brief Get the current value and ensure it is represented as a double.

		Get the current parameter as double.

		\CODE_SAMPLE_HDR
		// Implicit conversion
		double someDouble = myNode.someFeature.someParameter;
		or
		// Explicit conversion
		printf("%f", double(myNode.someFeature.someParameter));
		\endcode
	**/
	operator double();
	/**
		\brief Get the current value and ensure it is represented as a signed 
		integer.

		Get the current parameter as a signed integer.

		\note If the parameter is larger then +/- 31-bits the value will be 
		truncated.

		\CODE_SAMPLE_HDR
		// Implicit conversion
		int32_t someInt = myNode.someFeature.someParameter;
		or
		// Explicit conversion
		printf("%d", int32_t(myNode.someFeature.someParameter));
		\endcode
	**/
	operator int32_t();
	/**
		\brief Get the current value and ensure it is an unsigned integer.

		Get the current parameter as an unsigned integer.

		\note If the parameter is larger then 32-bits the value will be 
		truncated.

		\CODE_SAMPLE_HDR
		// Implicit conversion
		uint32_t someInt = myNode.someFeature.someParameter;
		or
		// Explicit conversion
		printf("%u", uint32_t(myNode.someFeature.someParameter));
		\endcode
	**/
	operator uint32_t();
	/**
		\brief Change our integer based parameter and optionally update the 
		non-volatile default value.

		\param[in] newValue New value to send to the node.
		\param[in] makeNonVolatile [Optionally update the non-volatile power-on
		default as well]

		\CODE_SAMPLE_HDR
		// Set Value and non-volatile power-on default
		myNode.someFeature.someParameter.Value(-1000, true);
		// Set Value only
		myNode.someFeature.someParameter.Value(-1000);
		\endcode

		\note Care should be taken to not excessively write the non-volatile
		parameter as the memory has a write count limitation.

		\see sFnd::ValueSigned::operator= For simplified syntax to write
		the run-time value.
	**/
	void Value(int32_t newValue, bool makeNonVolatile = false);
	/**
		\brief Get the current parameter value as an integer.

		\param[in] getNonVolatile If set, the value returned will be the
		power-on non-volatile value.

		\CODE_SAMPLE_HDR
		// Get and print the run-time value
		cout << myNode.Limits.SoftLimit0.Value() << endl;
		// Get and print the power-on default non-volatile value
		cout << myNode.Limits.SoftLimit0.Value(true) << endl;
		\endcode

		\note Care should be taken to not excessively write the non-volatile
		parameter as the memory has a write count limitation.
	**/
	int32_t Value(bool getNonVolatile=false);
	/** \cond INTERNAL_DOC **/
	// - - - - - - - - - - - - - - - - - - -
	// Common overrides for signed integers
	// - - - - - - - - - - - - - - - - - - -
	int32_t operator|(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue | arg));
	}
	int32_t operator&(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue & arg));
	}
	int32_t operator^(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue ^ arg));
	}
	int32_t operator+(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue + arg));
	}
	int32_t operator-(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue - arg));
	}
	int32_t operator*(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue * arg));
	}
	int32_t operator/(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue / arg));
	}
	int32_t operator|=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue | arg));
	}
	int32_t operator&=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue & arg));
	}
	int32_t operator^=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue ^ arg));
	}
	int32_t operator+=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue + arg));
	}
	int32_t operator-=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue - arg));
	}
	int32_t operator*=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue * arg));
	}
	int32_t operator/=(int32_t arg) {
		return(static_cast<int32_t>(m_currentValue / arg));
	}

	// Common overrides for unsigned integers

	int32_t operator|(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue | arg));
	}
	int32_t operator&(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue & arg));
	}
	int32_t operator^(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue ^ arg));
	}
	int32_t operator+(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	int32_t operator-(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	int32_t operator*(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	int32_t operator/(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}
	int32_t operator|=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue | arg));
	}
	int32_t operator&=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue & arg));
	}
	int32_t operator^=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue ^ arg));
	}
	int32_t operator+=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	int32_t operator-=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	int32_t operator*=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	int32_t operator/=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}

	// Common overrides for doubles
	int32_t operator+(double arg) {
		return(static_cast<int32_t>(m_currentValue + arg));
	}
	int32_t operator-(double arg) {
		return(static_cast<int32_t>(m_currentValue - arg));
	}
	int32_t operator*(double arg) {
		return(static_cast<int32_t>(m_currentValue * arg));
	}
	int32_t operator/(double arg) {
		return(static_cast<int32_t>(m_currentValue / arg));
	}
	int32_t operator+=(double arg) {
		return(static_cast<int32_t>(m_currentValue + arg));
	}
	int32_t operator-=(double arg) {
		return(static_cast<int32_t>(m_currentValue - arg));
	}
	int32_t operator*=(double arg) {
		return(static_cast<int32_t>(m_currentValue * arg));
	}
	int32_t operator/=(double arg) {
		return(static_cast<int32_t>(m_currentValue / arg));
	}
	/** \endcond **/

protected:
													/** \cond INTERNAL_DOC **/
	// Construction to wire to our node object
	ValueSigned(INode &node, nodeparam pNum);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueUnsigned class
/**
	\brief Unsigned 32-bit Integer Parameter Object.

	This class object implements access to unsigned integer parameters within
	the node.

	For most applications the following code sequence samples illustrate
	the commonly used functions.

	\CODE_SAMPLE_HDR
	// Setting a value
	myNode.someFeature.someParameter = 12345;

	// Getting a value
	uint32_t myVal = myNode.someFeature.someParameter;

	// Using a parameter in a calculation
	uint32_t myCalVal = myNode.someFeature.someParameter.Value() + 1000;

	// Making a copy of a value. This copy will run independently of the
	// node's reference. For example, myCopy.Refresh() will only
	// update myCopy's internal value.
	ValueUnsigned myCopy = myNode.someFeature.someParameter;

	// Making a short-cut reference. This sometimes can reduce the typing
	// requirements for using a parameter. In this example, aRef for
	// all intents and purposes is the same as myNode.aFeature.aParameter.
	ValueUnsigned &myRef = myNode.someFeature.someParameter;
	myRef.Refresh();
	// is the same as
	myNode.someFeature.someParameter.Refresh();
	\endcode

	\see [Value Parameter Overview](@ref ValueSect) for more information on ValueXXXX classes.
**/
class MN_EXPORT ValueUnsigned : public ValueBase
{
													/** \cond INTERNAL_DOC **/
	friend class IInfo;
	friend class IHoming;
	friend class ILimits;
	friend class IMotion;
	friend class IMotionAdv;
private:
	uint32_t m_lastValue;
	uint32_t m_currentValue;
													/** \endcond **/
public:
	/**
		\brief Query the node and obtain a new copy of the parameter.

		Query the node and obtain a new copy of the parameter.


		\CODE_SAMPLE_HDR
		//Refresh a volatile parameter before reading
		mynode.someFeature.someParameter.Refresh();
		\endcode
	**/
	void Refresh();

	/**
		\brief Set a new unsigned value using the assignment operator.

		This is the most common way to change an integer based parameter.

		\CODE_SAMPLE_HDR
		myNode.someFeature.someParameter = 1000;
		\endcode

		\see sFnd::ValueSigned::Value For an explicit way of setting
		a parameter as well as it's power-on default value.
		**/
		uint32_t operator=(uint32_t newValue);

	/**
		\brief Get the current parameter and ensure it is represented as a 
		double.

		Get the current parameter and format it as a double.

		\CODE_SAMPLE_HDR
		// Implicit conversion
		double someDouble = myNode.someFeature.someParameter;
		or
		// Explicit conversion
		printf("%f", double(myNode.someFeature.someParameter));
		\endcode
	**/
	operator double();

	/**
		\brief Get the current value and ensure it is an integer.

		Get the current value and ensure it is an integer.

		\note If the parameter is larger then 31-bits the value will be 
		truncated or undefined.

		\CODE_SAMPLE_HDR
		// Implicit conversion
		uint32_t someInt = myNode.someFeature.someParameter;
		or
		// Explicit conversion
		printf("%u", int32_t(myNode.someFeature.someParameter));
		\endcode
	**/
	operator uint32_t();

	/**
		\brief Change our integer based parameter and optionally update the
		non-volatile default value.

		Change our integer based parameter and optionally update the
		non-volatile default value.

		\param[in] newValue New value to send to the node.
		\param[in] makeNonVolatile [Optionally update the non-volatile power-on
		default as well]

		\CODE_SAMPLE_HDR
		// Set Value and non-volatile power-on default
		myNode.someFeature.someParameter.Value(2000, true);
		// Set Value only
		myNode.someFeature.someParameter.Value(2000);
		\endcode

		\note Care should be taken to not excessively write the non-volatile
		parameter as the memory has a write count limitation.

		\see sFnd::ValueSigned::operator= For simplified syntax to write
		the run-time value.
	**/
	void Value(uint32_t newValue, bool makeNonVolatile = false);

	/**
		\brief Get the current parameter value as an integer.

		Get the current parameter value as an integer.

		\param[in] getNonVolatile If set, the value returned will be the
		power-on non-volatile value.

		\CODE_SAMPLE_HDR
		// Get and print the run-time value
		cout << myNode.Limits.SoftLimit0.Value() << endl;
		// Get and print the power-on default non-volatile value
		cout << myNode.Limits.SoftLimit0.Value(true) << endl;
		\endcode

		\note Care should be taken to not excessively write the non-volatile
		parameter as the memory has a write count limitation.
	**/
	uint32_t Value(bool getNonVolatile = false);
	/** \cond INTERNAL_DOC **/
	// - - - - - - - - - - - - - - - - - - -
	// Common overrides for signed integers
	// - - - - - - - - - - - - - - - - - - -
	uint32_t operator|(int32_t arg) {
		return(m_currentValue | arg);
	}
	uint32_t operator&(int32_t arg) {
		return(m_currentValue & arg);
	}
	uint32_t operator^(int32_t arg) {
		return(m_currentValue ^ arg);
	}
	uint32_t operator+(int32_t arg) {
		return(m_currentValue + arg);
	}
	uint32_t operator-(int32_t arg) {
		return(m_currentValue - arg);
	}
	uint32_t operator*(int32_t arg) {
		return(m_currentValue * arg);
	}
	uint32_t operator/(int32_t arg) {
		return(m_currentValue / arg);
	}
	uint32_t operator|=(int32_t arg) {
		return(m_currentValue | arg);
	}
	uint32_t operator&=(int32_t arg) {
		return(m_currentValue & arg);
	}
	uint32_t operator^=(int32_t arg) {
		return(m_currentValue ^ arg);
	}
	uint32_t operator+=(int32_t arg) {
		return(m_currentValue + arg);
	}
	uint32_t operator-=(int32_t arg) {
		return(m_currentValue - arg);
	}
	uint32_t operator*=(int32_t arg) {
		return(m_currentValue * arg);
	}
	uint32_t operator/=(int32_t arg) {
		return(m_currentValue / arg);
	}

	// Common overrides for unsigned integers

	uint32_t operator|(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue | arg));
	}
	uint32_t operator&(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue & arg));
	}
	uint32_t operator^(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue ^ arg));
	}
	uint32_t operator+(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	uint32_t operator-(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	uint32_t operator*(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	uint32_t operator/(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}
	uint32_t operator|=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue | arg));
	}
	uint32_t operator&=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue & arg));
	}
	uint32_t operator^=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue ^ arg));
	}
	uint32_t operator+=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	uint32_t operator-=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	uint32_t operator*=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	uint32_t operator/=(uint32_t arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}

	// Common overrides for doubles
	uint32_t operator+(double arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	uint32_t operator-(double arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	uint32_t operator*(double arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	uint32_t operator/(double arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}
	uint32_t operator+=(double arg) {
		return(static_cast<uint32_t>(m_currentValue + arg));
	}
	uint32_t operator-=(double arg) {
		return(static_cast<uint32_t>(m_currentValue - arg));
	}
	uint32_t operator*=(double arg) {
		return(static_cast<uint32_t>(m_currentValue * arg));
	}
	uint32_t operator/=(double arg) {
		return(static_cast<uint32_t>(m_currentValue / arg));
	}
	/** \endcond **/

protected:
													/** \cond INTERNAL_DOC **/
	// Construction to wire to our node object
	ValueUnsigned(INode &node, nodeparam pNum);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueString class
/**
	\brief String Parameter Object.

	This class object implements access to various string oriented
	parameters within the node.

	For most applications the following code sequence samples illustrate
	the commonly used functions.

	\CODE_SAMPLE_HDR
	// Setting a value
	myNode.someFeature.someParameter = 12345;

	// Getting a value
	String myVal = myNode.someFeature.someParameter;

	// Using a parameter in a calculation
	String myCalVal = myNode.someFeature.someParameter.Value() + 1000;

	// Making a copy of a value. This copy will run independently of the
	// node's reference. For example, myCopy.Refresh() will only
	// update myCopy's internal value.
	ValueString myCopy = myNode.someFeature.someParameter;

	// Making a short-cut reference. This sometimes can reduce the typing
	// requirements for using a parameter. In this example, aRef for
	// all intents and purposes is the same as myNode.aFeature.aParameter.
	ValueString &myRef = myNode.someFeature.someParameter;
	myRef.Refresh();
	// is the same as
	myNode.someFeature.someParameter.Refresh();
	\endcode

	\see [Value Parameter Overview](@ref ValueSect) for more information on ValueXXXX classes.
**/
class MN_EXPORT ValueString : public ValueBase
{
													/** \cond INTERNAL_DOC **/
	friend class IInfo;
	friend class CPMinfo;
protected:
	enum _srcs {
		/**
			String is the user ID
		**/
		STR_USERID,
		/**
			String is located in the defined parameter.
		**/
		STR_PARAM,
		/**
			String is the config file name
		**/
		STR_CONFIG_NAME,
		/**
			Firmware Version String
		**/
		STR_FW_VERSION,
		/**
			Hardware Version String
		**/
		STR_HW_VERSION,
		/**
			Return the model string
		**/
		STR_MODEL_STR
	};

	// C++ hack to create a constant
	enum _MAX_STR { MAX_STR = MN_UI_STR_BUF_SIZE };
	// Where to find string
	enum _srcs  m_location;

	char m_lastValue[MAX_STR];
	char m_currentValue[MAX_STR];
													/** \endcond **/
public:
	/**
		\brief Query the node and obtain a new copy of the parameter.

		Query the node and obtain a new copy of the parameter.


		\CODE_SAMPLE_HDR
		//Refresh a volatile parameter before reading
		mynode.someFeature.someParameter.Refresh();
		\endcode
	**/
	virtual void Refresh() = 0;

	/**
		\brief Update the string parameter.

		\param[in] newValue New C-String to use for update.

		Update the string parameter with \a newValue.

		\CODE_SAMPLE_HDR
		myNode.Info.UserID.Value("My Node");
		\endcode
	**/
	virtual void Value(const char *newValue) = 0;

	/**
		\brief Get the current string parameter.

		\return The current string parameter.

		Return a pointer to a read-only buffer that contains the current
		value. 

		\CODE_SAMPLE_HDR
		cout << myNode.Info.UserID.Value() << endl;
		\endcode
	**/
	virtual const char *Value() = 0;

	/**
		\brief Update the string value using the assignment operator.

		\param[in] newValue Incoming string pointer.

		This is the most common way to change a string based parameter.

		\CODE_SAMPLE_HDR
		myNode.Info.UserID = "My Node";
		\endcode
	**/
	const char *operator=(const char *newValue);

	/**
		\brief Get a Pointer to the Current String Value.

		Get a pointer to the read-only current string value.

		\CODE_SAMPLE_HDR
		const char *myParam = myNode.Info.UserID;
		\endcode
	**/
	operator const char *();

protected:
													/** \cond INTERNAL_DOC **/
	// Construction to wire to our node object
	ValueString(INode &node, nodeparam pNum, enum _srcs location);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueStatus class
/**
	\brief Status Type Register Node Parameter Access Object.

	This class object implements access to the various types of status register/masks within the
	node.

	Different Types of Node status registers are useful for determining whether a particular
	status field is currently asserted, has been asserted since the last read, or has been cleared
	since the last read.  The different types of Status Registers are:

	- [Real-Time](@ref sFnd::IStatus::RT) Which represents the current state of the Status Register.
	- [Rising](@ref sFnd::IStatus::Rise) Which tracks rising edges of status fields. Fields in this 
	register are set when the [Real-Time](@ref sFnd::IStatus::RT) fields transition from 0 to 1.
	- [Falling](@ref sFnd::IStatus::Fall) Which tracks falling edges of status fields. Fields in this 
	register are set when the [Real-Time](@ref sFnd::IStatus::RT) fields transition from 1 to 0.
	- [Accumulating](@ref sFnd::IStatus::Accum) Which represents the Or accumulation of the 
	[Real-Time](@ref sFnd::IStatus::RT) register.

	These registers are internally clear-on-read within the Node.  This object provides a
	caching mechanism allowing the application to control when the stored value of this register is 
	cleared/refreshed.  This allows multiple threads which are competing for different Status event to 
	coexist without interfering with each other.

	This object also provides access to Status masks used to generate attentions.

	\see sFnd::IStatus::RT
	\see sFnd::IStatus::Rise
	\see sFnd::IStatus::Fall
	\see sFnd::IStatus::Accum
	\see cpmStatusRegFlds for a complete list of ClearPath-SC Status Fields
	\see \ref AttentionsPage
**/
class MN_EXPORT ValueStatus : public ValueBase
{
													/** \cond INTERNAL_DOC **/
	friend class IStatus;
	friend class IAttnNode;
private:
	mnStatusReg m_lastValue;
	mnStatusReg m_currentValue;
	mnStatusReg m_testResult;
	bool m_clearOnRead;
													/** \endcond **/
public:
	/**
		\brief Update our Status Register from the Node.

		This function will atomically read and accumulate Rising, Falling, and Accumulating registers to prevent the loss of state information when accessed by 
		multiple threads.  This means refreshing these registers will not clear any currently asserted bits. Use one of the [TestAndClear](@ref sFnd::ValueStatus::TestAndClear) 
		functions to combine with testing to prevent possible information loss from clearing the entire register at once.

		For the Real-Time register this function simply replaces the value of the stored register to the current status register within the Node.

		\CODE_SAMPLE_HDR
		//Refresh The Accumulating register
		mynode.Status.Accum.Clear();		//Clear the accumulating register so we only capture accumulated events since the last read
		mynode.Status.Accum.Refresh();		//Refresh the register

		//Refresh the Register (No clear is needed since the Real Time register simply overwrites the REgister to be the current value)
		mynode.Status.RT.Refresh();			

		\endcode
	**/
	void Refresh();

	/**
		\brief Read, Test and Clear Accumulated <i>Status Register</i> state.

		\param[in] mask fields to test for.
		\param[out] result fields that were cleared.
		\return true if one or more were true.
	
		This function is a thread safe way of updating and atomically testing 
		and clearing fields from the accumulating <i>Status Registers</i> ()Rise, Fall, Accum). The 
		\a result is updated with all the fields detected by the mask. This 
		is useful when testing multiple fields.  This function is not needed with the Real-Time register.

		\CODE_SAMPLE_HDR
		//Create a Status register mask to test the Accumulating register
		//Test for move done, A from start, and B from end.
		mnStatusReg mymask;
		mymask.cpm.MoveDone = 1;
		mymask.cpm.BFromEnd = 1;
		mymask.cpm.AFromStart = 1;

		//Test and clear these three fields
		mnStatusReg myResult;
		if(mynode.Status.Accum(mymask,myResult))
		{
			if(myResult.cpm.MoveDone)
			{ Move Done occured}
			if(myResult.cpm.BFromEnd)
			{ B from end occured}
			if(myResult.cpm.AFromStart)
			{ A from start occured}
		}
		//At this point the three fields are now clear in our stored copy of the Accum register Refresh to find more events 
		\endcode
	**/
	bool TestAndClear(const mnStatusReg &mask, mnStatusReg &result);

	/**
		\brief Read and simple test and clear for single fields

		\param[in] mask <i>Status Register</i> union with the desired field to 
		test and clear.
		\return True if the desired field was set.

		This function is a thread safe way of updating and atomically testing
		and clearing a field from the accumulating <i>Status Registers</i> ()Rise, Fall, Accum).
		This function is not needed with the Real-Time register.

		\note You should only use this function to test a single field. You cannot
		determine which field was set if multiple fields are set in the mask.

		\see sFnd::ValueStatus::TestAndClear(const mnStatusReg &, mnStatusReg &) for multiple
		field test and clear.

		\CODE_SAMPLE_HDR
		//Create a Status register mask to test the Accumulating register
		//Test for move done
		mnStatusReg mymask;
		mymask.cpm.MoveDone = 1;

		//Test and clear the move done field
		if(mynode.Status.Accum(mymask))
		{Move Done occured}
		
		//At this point the three fields are now clear in our stored copy of the Accum register Refresh to find more events
		\endcode
	**/
	bool TestAndClear(const mnStatusReg &mask);

	/**
		\brief Clear the accumulated state

		This function is used to clear out accumulated status information
		from the internal copy.   This function is not needed for the Real-Time register. 

		\CODE_SAMPLE_HDR
		//Clear, and refresh The Accumulating register
		mynode.Status.Accum.Clear();
		mynode.Status.Accum.Refresh();	

		\endcode
	**/
	void Clear();

	/**
		\brief Assignment operator to update value with #mnStatusReg union value.

		Assignment operator to update value with #mnStatusReg union value.
		This function should only be used when setting up Status masks for attentions.
		
		\see sFnd::IAttnNode::Mask
		\see sFnd::IAttnNode::StatusMask

		\CODE_SAMPLE_HDR
		//Create a Status register mask to set the Attentionable Status Mask to.
		//Here we are setting up attentions for Move done, A from start, and B from end
		// Only the lower 32 bits are attentionable.
		mnStatusReg mymask;
		mymask.cpm.MoveDone = 1;
		mymask.cpm.BFromEnd = 1;
		mymask.cpm.AFromStart = 1;

		//Set the Node's Attentionable Status Mask to our mask.
		mynode.Adv.Attn.Mask = mymask;

		\endcode
	**/
	mnStatusReg &operator=(mnStatusReg &newValue);

	/**
		\brief Copy operator to return a copy of the current status type 
		register.

		Copy operator to return a copy of the current status type
		register.
	**/
	operator mnStatusReg();

	/**
		\brief Return a copy of the last acquired Status Type Register.

		Return a copy of the last acquired Status Type Register.
	**/
	mnStatusReg Value();

	/**
		\brief Set the <i>Status Register</i> type register with the new value.
	
		Update the internal register value within the Node with newValue.
		This function should only be used when setting up Status masks for attentions.
		
		\see sFnd::IAttnNode::Mask
		\see sFnd::IAttnNode::StatusMask

		\CODE_SAMPLE_HDR
		//Create a Status register mask to set the Attentionable Status Mask to.
		//Here we are setting up attentions for Move done, A from start, and B from end
		// Only the lower 32 bits are attentionable.
		mnStatusReg mymask;
		mymask.cpm.MoveDone = 1;
		mymask.cpm.BFromEnd = 1;
		mymask.cpm.AFromStart = 1;

		//Set the Node's Attentionable Status Mask to our mask.
		mynode.Adv.Attn.Mask.Value(mymask);

		\endcode
	**/
	void Value(const mnStatusReg &newValue);

	/**
		\brief Return a copy of the last acquired value.

		Return a copy of the last acquired value.

		\return Copy of the last updated value.
	**/
	mnStatusReg Last();

protected:
												/** \cond INTERNAL_DOC **/
	// Construction to wire to our node object
	ValueStatus(INode &node, nodeparam pNum, bool clearOnReadType);
												/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueAlert class
/**
	\brief Alert Type Register Node Parameter Access Object.

	This class object implements access to the alert register and the attentionable alert mask within the
	node.

	The motor's firmware and hardware contains many mechanisms to protect
	the motor and the application from problems that may occur. When
	an operation-blocking condition occurs, it is known as an <i>Alert</i>, or
	sometimes called a <i>Shutdown</i>. The shutdown is logged in the
	non-volatile memory with a time-stamp for review with the ClearView application.
	These alerts are stored locally within the node in a ninety-six bit [alertReg](@ref alertReg) struct.
	
	The Attentionable alert mask is the mask which determines what alert bits will show in the User Alert field of the #mnStatusReg.

	The following is an example

	\see sFnd::IStatus::Alerts 
	\see sFnd::IAttnNode::AlertMask
	\see \ref AttentionsPage
	\CODE_SAMPLE_HDR
	
	//This example code will check to see if a node has any current alerts and 
	//print out the state string of any alerts found

	//This creates an Empty String for us to put our Alert State String into
	char alertList[256];

	// get a reference to our ValueAlert Object
	ValueAlert &myNodesAlert = myNode.Status.Alerts;

	// Refresh the Real-Time Status, and Alert Register
	theNode.Status.RT.Refresh();
	myNodesAlert.Refresh();


	// Check the Real-Time Status Register's "AlertPresent" bit
	// The bit is set true if there are alerts in the alert register
	if (theNode.Status.RT.Value().cpm.AlertPresent) 
	{
		printf("Node has alerts!\n");
	}
	else 
	{
		printf("Node has no alerts!\n");
	}

	// get an alertReg reference, and check directly for alerts
	if (myNodesAlert.Value().isInAlert()) 
	{
		// Put a text description of all alert bits set into our empty string, then print the string
		myNodesAlerts.Value().StateStr(alertList, 256);
		printf("Node has alerts! Alerts: %s", alertList);

	// Check for specific alerts by accessing the cpmAlertFlds within the alertReg Struct
		if (myNodesAlert.Value().cpm.TrackingShutdown)
		{
			printf("Tracking Shutdown\n");
		} 
	}
	
	\endcode

	\see _cpmAlertFlds for a complete list of Possible Alerts
**/
class MN_EXPORT ValueAlert : public ValueBase
{
												/** \cond INTERNAL_DOC **/
	friend class IStatus;
	friend class IAttnNode;
private:
	alertReg m_lastValue;
	alertReg m_currentValue;
	alertReg m_testResult;
	bool m_clearOnRead;
												/** \endcond **/
public:
	/**
		\brief Query the node for a new copy of the underlying alert register.

		This function will update the stored value to reflect current state 
		of the node's Alert Register.
	**/
	void Refresh();
													/** \cond INTERNAL_DOC **/
	/**
		\brief Read, Accumulate and Test and Clear Accumulated Alert Type 
		Register state.

		\param[in] mask Fields to test for
		\param[out] result Fields that were cleared
		\return true if one or more were true

		This function is a thread safe way of atomically testing and clearing
		fields from accumulating <i>Status Registers</i>. The /a result
		union is updated with all the fields detected by the mask. This is useful
		when testing multiple fields.
		**/
	bool TestAndClear(const alertReg &mask, alertReg &result);

	/**
		\brief Clear the accumulated state

		This function is used to clear out the accumulated alert information from
		the internal copy.
	**/
	void Clear();
													/** \endcond  **/
	/**
		\brief Assignment operator to update value with #alertReg union value.

		Assignment operator to update value with #alertReg union value.
		This function should only be used when setting up the Attentionable Alert mask for attentions.

		/see sFnd::IAttnNode::AlertMask

		\CODE_SAMPLE_HDR
		//Create a Alert register mask to set the Attentionable Alert Mask to.
		//Here we are setting up attentions for a tracking error shutdown, a RMS Shutdown, 
		// and a soft limit shutdown
		alertReg mymask;
		mymask.cpm.TrackingShutdown = 1;
		mymask.cpm.RMSOverloadShutdown = 1;
		mymask.cpm.SoftLimitsExceeded = 1;

		//Set the Node's Attentionable Status Mask to our mask.
		mynode.Adv.Attn.AlertMask = mymask;

		\endcode
	**/
	alertReg &operator=(alertReg &newValue);

	/**
		\brief Copy operator to return a copy of the current alert type register.

		Copy operator to return a copy of the current alert type register.
	**/
	operator alertReg();

	/**
		\brief Return a copy of the last acquired Alert Type Register.

		Copy operator to return a copy of the current alert type register.
	**/
	alertReg Value();

	/**
		\brief Set the <i>Alert Register</i> type register with the new value.

		\param[in] newValue The value to update.

		Update the internal register value within the Node with newValue.
		This function should only be used when setting up the Attentionable Alert mask for attentions.

		/see sFnd::IAttnNode::AlertMask

		\CODE_SAMPLE_HDR
		//Create a Alert register mask to set the Attentionable Alert Mask to.
		//Here we are setting up attentions for a tracking error shutdown, a RMS Shutdown,
		// and a soft limit shutdown
		alertReg mymask;
		mymask.cpm.TrackingShutdown = 1;
		mymask.cpm.RMSOverloadShutdown = 1;
		mymask.cpm.SoftLimitsExceeded = 1;

		//Set the Node's Attentionable Status Mask to our mask.
		mynode.Adv.Attn.AlertMask = mymask;

		\endcode
	**/
	void Value(const alertReg &newValue);

	/**
		\brief Return a copy of the prior acquired value.

		Return a copy of the prior acquired value.
	**/
	alertReg Last() { return m_lastValue; }
protected:
												/** \cond INTERNAL_DOC **/
	// Construction to wire to our node object
	ValueAlert(INode &node, nodeparam pNum, bool clearOnReadType);
												/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValuePowerReg class
/**
\brief Power Status Register Node Parameter Access Object.

This class object implements access to the power state register.

The motor's firmware monitors the power connections continuously. This
register shows the currently detected connections and any anomalies detected
during operations. These include loose phase wiring and which terminals
the power is attached to.

The following is an example

\todo see
\CODE_SAMPLE_HDR

//This example code will check and return the current power conditions and
//print out the state string of any issues found.

//This creates an Empty String for us to put our state string into
char pwrInfo[256];

// get a reference to our ValuePowerReg Object
ValuePowerReg &myPower = myNode.Status.Power;

// Refresh the Real-Time Status, and Alert Register
theNode.Status.Power.Refresh();
myPower.Refresh();


// Check that there has been no disruptions in the AC supply.
// The bit is set true if there are alerts in the alert register
if (theNode.Status.Power.Value().fld.InBusLoss)
{
printf("Node's power bus has been removed!\n");
}
else
{
printf("Node has no bus supply problems!\n");
}

// Put a text description of all alert bits set into our empty string, then print the string
myNodesAlerts.Value().StateStr(pwrInfo, 256);
printf("Node power state: %s", pwrInfo);


\endcode

\see _mnPowerFlds for a complete list of Possible Alerts
**/
class MN_EXPORT ValuePowerReg : public ValueBase
{
	/** \cond INTERNAL_DOC **/
	friend class IStatus;
	friend class IAttnNode;
private:
	mnPowerReg m_lastValue;
	mnPowerReg m_currentValue;
	mnPowerReg m_testResult;
	bool m_clearOnRead;
	/** \endcond **/
public:
	/**
	\brief Query the node for a new copy of the underlying alert register.

	This function will update the stored value to reflect current state
	of the node's Alert Register.
	**/
	void Refresh();
	/** \cond INTERNAL_DOC **/
	/**
	\brief Read, Accumulate and Test and Clear Accumulated Power Type
	Register state.

	\param[in] mask Fields to test for
	\param[out] result Fields that were cleared
	\return true if one or more were true

	This function is a thread safe way of atomically testing and clearing
	fields from accumulating <i>Power Status Register</i>. The /a result
	union is updated with all the fields detected by the mask. This is useful
	when testing multiple fields.
	**/
	bool TestAndClear(const mnPowerReg &mask, mnPowerReg &result);

	/**
	\brief Clear the accumulated state

	This function is used to clear out the accumulated status information from
	the internal copy.
	**/
	void Clear();
	/** \endcond  **/

	/**
	\brief Copy operator to return a copy of the current power type register.

	Copy operator to return a copy of the current power type register.
	**/
	operator mnPowerReg();

	/**
	\brief Return a copy of the last acquired power Type Register.

	Copy operator to return a copy of the current power type register.
	**/
	mnPowerReg Value();

	/**
	\brief Return a copy of the prior acquired value.

	Return a copy of the prior acquired value.
	**/
	mnPowerReg Last() { return m_lastValue; }
protected:
	/** \cond INTERNAL_DOC **/
	// Construction to wire to our node object
	ValuePowerReg(INode &node, nodeparam pNum, bool clearOnReadType);
	/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// ValueOutReg class
/**
	\cond SC_EXPERT
	\brief Output Register Node Parameter Access Object.

	This class object implements the access to a #mnOutReg type register.
**/
class MN_EXPORT ValueOutReg : public ValueBase
{
											
	friend class IOuts;

private:
	mnOutReg m_lastValue;
	mnOutReg m_currentValue;
	mnOutReg m_testResult;
	bool m_clearOnRead;
	bool m_writeable;
											
public:
	/**
		\brief Query the node of a new copy of the underlying output register.

		The update will atomically read and accumulate clear-on-read style registers
		to prevent the loss of state information when accessed by multiple
		threads. 
	**/
	void Refresh();
	/**
		\brief Clear fields from the output register state

		Use this function to atomically update the output register from multiple
		threads to avoid state corruption.
	**/
	void Clear(const mnOutReg &mask);
	/**
		\brief Set fields from the output register state

		\param[in] mask Fields to set.

		Use this function to atomically update the output register from multiple
		threads to avoid state corruption.

		\CODE_SAMPLE_HDR
		mnOutReg bitsToSet;
		bitsToSet.cpm.EnableReq = true;
		bitsToSet.cpm.ResetTimer = true;
		myNode.Outs.User.Set(bitsToSet);
		\endcode
	**/
	void Set(const mnOutReg &mask);

	/**
		\brief Update the output register using an mnOutReg.

		Update the output register using an mnOutReg.

		\CODE_SAMPLE_HDR
		const char *myParam = myNode.Info.UserID;
		\endcode
		**/
	mnOutReg &operator=(mnOutReg &newValue);
	/**
		\brief Copy the current state of the <i>Output Register</i>
		to another mnOutReg instance.

		Copy the current state of the <i>Output Register</i>
		to another mnOutReg instance.
	**/
	operator mnOutReg();
	/**
		\brief Copy the current state of the <i>Output Register</i> as a
		bit-wide integer value.

		Copy the current state of the <i>Output Register</i> as a
		bit-wide integer value.
	**/
	operator double();
	/** 
		\brief Copy the current state of the <i>Output Register</i> as a
		bit-wide integer value.

		Copy the current state of the <i>Output Register</i> as a
		bit-wide integer value.
	**/
	operator int32_t();
	/**
		\brief Copy the current state of the <i>Output Register</i> as a
		bit-wide integer value.

		Copy the current state of the <i>Output Register</i> as a
		bit-wide integer value.
	**/
	operator uint32_t();

	/**
		\brief Update the <i>Output Register</i> to a new value from an 
		mnOutReg union.
	
		\param[in] newValue Reference to a mnOutReg instance.

		Update the <i>Output Register</i> to a new value from an
		mnOutReg union.
	**/ 
	void Value(const mnOutReg &newValue);
	
	/**
		\brief Return a copy of the current <i>Output Register</i> state.

		Return a copy of the current <i>Output Register</i> state.
	**/
	mnOutReg Value();

	/**
		\brief Return a copy of the last updated <i>Output Register</i> state.

		Return a copy of the last updated <i>Output Register</i> state.
	**/
	mnOutReg Last() { return m_lastValue; }

protected:
													
	// Construction to wire to our node object
	ValueOutReg(INode &node, nodeparam pNum,
		bool clearOnReadType, bool writeable);
													
};
							/**   \endcond **/
//						  													  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueAppConfigReg class
														/** \cond SC_EXPERT **/
/**
	\brief Application Configuration Parameter Object.

	This class object implements the access to an mnAppConfigReg type register. 

	\copydetails _mnAppConfigReg

	\note Care should be taken accessing these registers as configuration file
	loads may modify these settings.

	\see _cpmAppConfigReg For details about the ClearPath-SC application
	configuration.
**/
class MN_EXPORT ValueAppConfigReg : public ValueBase
{
													/**  INTERNAL_DOC **/
	friend class ISetupEx;
	friend class CPMmotionAdv;

private:
	mnAppConfigReg m_lastValue;
	mnAppConfigReg m_currentValue;
	mnAppConfigReg m_testResult;
													/**  **/
public:
	/**
		\brief Query the node of a new copy of the underlying <i>Application
		Configuration</i> register.

		Query the node of a new copy of the underlying <i>Application
		Configuration</i> register.
	**/
	void Refresh();
	/**
		\brief Clear fields in this register in thread safe manner.

		\param[in] mask Fields to clear out of the register

		Clear fields in this register in thread safe manner.
	**/
	void Clear(const mnAppConfigReg &mask);
	/**
		\brief Set fields in this register in thread safe manner.

		\param[in] mask Fields to set in the register

		Set fields in this register in thread safe manner.
	**/
	void Set(const mnAppConfigReg &mask);

	/**
		\brief Copy an existing instance to our state via the 
		assignment operator.

		Copy an existing instance to our state via the
		assignment operator.
	**/
	mnAppConfigReg &operator=(mnAppConfigReg &newValue);

	/**
		\brief Copy our instance to another instance.

		Copy our instance to another instance.
	**/
	operator mnAppConfigReg();

	/**
		\brief Get our state as an integer represented in a double.

		Get our state as an integer represented in a double.
	**/
	operator double();

	/**
		\brief Get our state as an integer.

		Get our state as an integer.
	**/
	operator int32_t();

	/**
		\brief Get our state as an integer.

		Get our state as an integer.
	**/
	operator uint32_t();

	/**
		\brief Update our internal state from another instance.

		Update our internal state from another instance.
	**/
	void Value(const mnAppConfigReg &newValue);
	/**
		\brief Get a copy of our current state.

		Get a copy of our current state.
	**/
	mnAppConfigReg Value();

	/**
		\brief Return the last prior state of this register.

		Return the last prior state of this register.
	**/
	mnAppConfigReg Last() { return m_lastValue; }


protected:
													/**  INTERNAL_DOC **/
	// Update the internal value to new state
	void setNewVal(const mnAppConfigReg &newValue);
	// Construction to wire to our node object
	ValueAppConfigReg(INode &node, nodeparam pNum);
													/**  **/
};
													/** \endcond  **/
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ValueHwConfigReg class
													/** \cond SC_EXPERT **/
/**
	\brief Hardware Configuration Parameter Object.

	This class object implements the access to an mnHwConfigReg type register.

	\copydetails _mnHwConfigReg

	\note Care should be taken accessing these registers as configuration file
	loads may modify these settings.

	\see _cpmHwConfigReg For details about the ClearPath-SC hardware 
	configuration register.
**/
class MN_EXPORT ValueHwConfigReg : public ValueBase
{
													/**  INTERNAL_DOC **/
	friend class ISetupEx;

private:
	mnHwConfigReg m_lastValue;
	mnHwConfigReg m_currentValue;
	mnHwConfigReg m_testResult;
													/**  **/
public:
	/**
		\brief Query the node for a new copy of the underlying <i>Application
		Configuration</i> register.

		Query the node for a new copy of the underlying <i>Application
		Configuration</i> register.
	**/
	void Refresh();

	/**
		\brief Clear fields in this register in thread safe manner.

		\param[in] mask Fields to clear out of the register

		Clear fields in this register in thread safe manner.
	**/
	void Clear(const mnHwConfigReg &mask);

	/**
		\brief Set fields in this register in thread safe manner.

		\param[in] mask Fields to set in the register

		Set fields in this register in thread safe manner.
	**/
	void Set(const mnHwConfigReg &mask);


	/**
		\brief Copy an existing instance to our state via the
		assignment operator.

		\param[in] newValue The incoming instance

		Copy an existing instance to our state via the
		assignment operator.
	**/
	mnHwConfigReg &operator=(mnHwConfigReg &newValue);

	/**
		\brief Copy our internal state to another instance.

		Copy our internal state to another instance.
	**/
	operator mnHwConfigReg();

	/**
		\brief Get the numeric equivalent value of our register.

		Get the numeric equivalent value of our register formatted as 
		a double.
	**/
	operator double();


	/**
		\brief Get the numeric equivalent value of our register.

		Get the numeric equivalent value of our register as integer.
	**/
	operator int32_t();

	/**
		\brief Get the numeric equivalent value of our register.

		Get the numeric equivalent value of our register as an unsigned integer.
	**/
	operator uint32_t();

	/**
		\brief Get a copy of our current state.

		Get a copy of our current state.
	**/
	mnHwConfigReg Value();

	/**
		\brief Set this register to a new value

		\param[in] newValue New value to send to the node.

		Set this register to a new value.
	**/
	void Value(const mnHwConfigReg &newValue);

	/** 
		\brief Get a copy of the previous value.

		Get a copy of the previous value.
	**/
	mnHwConfigReg Last() { return m_lastValue; }

protected:
													/**  INTERNAL_DOC **/
	/// Construction to wire to our node object
	ValueHwConfigReg(INode &node, nodeparam pNume);
													/**  **/
};
														/** \endcond  **/
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	IBrakeControl class
/**
	\brief Brake Control Feature Interface.
	
	This class implements access to the brake control outputs of a SC-Hub Board.  If multiple SC-Hub boards are chained together on a single port, 
	only the first SC-Hub board's outputs will be controllable.

	The Brake Control feature can be used to automatically control a brake whenever a node is disabled or shutdown, or they can be used as general purpose outputs.
	This object also allows the application to override the brake to turn the brake off even when the node is disabled, allowing 
	the axis to move freely.  This class provides functions to turn the outputs on and off, as well as to check the current state of the outputs.

	Sometimes a brake can take time to engage so there is a Node Level [delay to disable](@ref sFnd::ISetup::DelayToDisableMsecs) parameter 
	which can be set to give the brake time to activate before the motor disables.


	\note When using #BRAKE_AUTOCONTROL mode, Brake_0 will activate and deactivate based on the state of the node connected to the CP0 connector on the SC-Hub Board.
	Similarly, Brake_1 will correspond to CP1 on the SC-Hub Board.

	\CODE_SAMPLE_HDR

	// Configure the SC hub's Brake_0 to be in auto control mode (the brake will automatically activate, and deactivate based on the state of node 0)
	myPort.BrakeControl.BrakeSetting(0,BRAKE_AUTOCONTROL);

	//Determine if the current Brake Setting for the SC-Hub Board's Brake_0 is BRAKE_AUTOCONTROL
	if(myPort.BrakeControl.BrakeSetting(0)==BRAKE_AUTOCONTROL)
	{  In auto control}
	else
	{ Not in auto control (The brake could be overridden)}

	//Determine if the Brake_0 output on the SC-Hub Board's is engaged
	if(myPort.BrakeControl.BrakeEngaged(0))
	{  Brake Engaged }
	else
	{  Brake Not Engaged}

	// Turn on the SC hub's Brake_1 output as a general purpose Output.
	myPort.BrakeControl.BrakeSetting(1,GPO_ON);
	
	\endcode

	\see  #_BrakeControls for the complete list of brake modes

**/
class MN_EXPORT IBrakeControl : public IObjWithPort
{
													/** \cond INTERNAL_DOC **/
	friend class IPort;
													/** \endcond **/
public:
	/**
		\brief Setup how brake will function.

		This function configures the configures the brake output.


		\param[in] brakeNum Brake number to use [0..1] 0 corresponds to Brake_0 on a SC Hub, 1 corresponds to Brake_1.
		\param[in] brakeMode Brake mode to use. 

		\see  #_BrakeControls for the complete list of modes

	**/
	virtual void BrakeSetting(size_t brakeNum, BrakeControls brakeMode) = 0;
	/**
		\brief Get the brake control state.

		Get the brake control state.

		\param[in] brakeNum Brake number to use [0..1] 0 corresponds to Brake_0 on a SC Hub, 1 corresponds to Brake_1.
		
		\see  #_BrakeControls for the complete list of modes
		
	**/
	virtual BrakeControls BrakeSetting(size_t brakeNum) = 0;

	/**
		\brief Return the current brake state.

		Determine the curent state of the Brake.  A Brake is considered engaged when the brake is holding the axis.  This would correspond to zero current flow on a GPO.

		
		\param[in] brakeNum Brake number to use [0..1]; 0 corresponds to Brake_0 on a SC Hub, 1 corresponds to Brake_1.
		\return true if the brake is engaged.

	**/
	virtual bool BrakeEngaged(size_t brakeNum) = 0;

protected:
	/** \cond INTERNAL_DOC **/
	/* Construction */
	IBrakeControl(IPort &port);
public:
	virtual ~IBrakeControl() {}
	/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IGrpShutdown class
/**
	\brief Group Shutdown Feature Interface.

	This class implements access to the group shutdown feature which
	 allows an electrical connection or a change in a node's status to initiate a Group Shutdown Node Stop. 
	 This Group Shutdown will automatically propagate and initiate a Node Stop on all nodes in the port.
	 
	This class can configure how each node on the port will react to a Group Shutdown Node Stop, as well as whether 
	certain fields in the Node's mnStatusdReg will issue a group shutdown.  For instance, once a node goes [not ready](@ref cpmStatusRegFlds::NotReady)
	, a group shutdown can be called.

	Once a group shutdown is triggered the shutdown is sent to each node on the port. A group shutdown 
	is also automatically triggered upon the loss of power (negative edge) of the global stop input on the SC-Hub board.

	\CODE_SAMPLE_HDR

	// The following lines of code show an example of how to create a Shutdown info object.
	
	// First we create the status mask object consisting of which status events on the node will trigger a group shutdown.
	// In this case we are using "AlertPresent", "Disabled", and "Input A"

	mnStatusReg myShutdownStatusMask;						//Create Status mask
	myShutdownStatusMask.cpm.AlertPresent = 1;				//Set desired fields
	myShutdownStatusMask.cpm.Disabled = 1;
	myShutdownStatusMask.cpm.InA = 1;

	// Next we create the ShutdownInfo object we will use to configure the Node with
	ShutdownInfo myShutdownInfo;							//Create ShutdownInfo Object	
	myShutdownInfo.enabled = true;							// This sets the node to respond to Group Shutdowns
	myShutdownInfo.theStopType = STOP_TYPE_ABRUPT;			//Use an Abrupt Nodestop type for this node in response to a group shutdown
	myShutdownInfo.statusMask = myShutdownStatusMask;		// Configure the node to issue a group shutdown automatically based on our status mask

	//Finally we configure the Port's first Node (Node 0) with our shutdown object 
	myport.GrpShutdown.ShutdownWhen(0,myShutdownInfo);

	\endcode

	\see \ref CPMnodeStopPage

**/
class MN_EXPORT IGrpShutdown : public IObjWithPort
{
													/** \cond INTERNAL_DOC **/
	friend class IPort;
													/** \endcond **/
public:
	/**
		\brief Setup the group shutdown for this node.

		This function is used to setup how a node will react to a group shutdown, as well as
		which items in the node's status register will trigger a group shutdown.  

		\param[in] nodeIndex Node index. This corresponds to the location from
		the host on the network.
		\param[in] theInfo Settings to apply to this node.

		\see _ShutdownInfo for information on how to specify the stoptype, status mask, and
		whether the grpShutdown feature is enabled for this node

	**/
	virtual void ShutdownWhen(size_t nodeIndex, const ShutdownInfo &theInfo) = 0;
	/**
		\brief Get the group shutdown settings for this node.

		Retrieve the current #_ShutdownInfo for this node, including the stop type, status mask, 
		and whether the feature is enabled

		\param[in] nodeIndex Node index. This corresponds to the location from
		the host on the network.
		\param[out] theInfo Settings to apply to this node.

	**/
	virtual void ShutdownWhenGet(size_t nodeIndex, ShutdownInfo &theInfo) = 0;
	/**
		\brief Initiate a group shutdown.

		Trigger the Group Shutdown to occur via an API call.

	**/
	virtual void ShutdownInitiate() = 0;
protected:
													/** \cond INTERNAL_DOC **/
	// Construct our group shutdown implementation
	IGrpShutdown(IPort &port);
public:
	virtual ~IGrpShutdown() {}
													/** \endcond **/
};
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	IPortAdv class
/**
	\brief Advanced Port Features Interface.

	This implements the objects required to access any advanced features
	at the port level.  Currently, this class provides a function to trigger groups of Nodes on this Port.
**/
class MN_EXPORT IPortAdv : public IObjWithPort
{
												/** \cond INTERNAL_DOC **/
	friend class IPort;
												/** \endcond **/
public:
												
	/**
		\brief Access the Attention Handling feature object

		This object allows access to the advanced <i>Attention Request</i>
		feature. This feature allows for non-polled asynchronous event
		handling.
	**/
	IAttnPort &Attn;
													
	/**
		\brief Trigger a group of nodes to initiate their moves.

		\param[in] groupNumber Group to signal on the network attached to this
		port.

		If there are no waiting moves or nodes setup at this \a groupNumber,
		this command will be ignored with no actions taken.

		\see \ref CPMtriggerPage for more infomation on triggered moves
		\see sFnd::IMotionAdv for configuring/loading triggered moves
		\see sFnd::IMotionAdv::TriggerGroup to set/get a Node's trigger Group 

		\CODE_SAMPLE_HDR
		// This example will load some triggered moves onto a group of Nodes, then Trigger them at the Port Level

		myNode1.Motion.Adv.MovePosnStart(1000,false,true);  /Load a 1000 cnt relative triggered move onto Node 1
		myNode2.Motion.Adv.MovePosnStart(1000,false,true);  /Load a 1000 cnt relative triggered move onto Node 2
		myNode3.Motion.Adv.MovePosnStart(1000,false,true);  /Load a 1000 cnt relative triggered move onto Node 3

		//Set the Trigger group of Node's 1, 2, and 3
		myNode1.Motion.Adv.TriggerGroup(1);
		myNode2.Motion.Adv.TriggerGroup(1);
		myNode3.Motion.Adv.TriggerGroup(2);

		//Trigger the Loaded Moves
		myPort.Adv.TriggerMovesInGroup(1);	//This will trigger Node 1 and 2 (Group 1)
		myPort.Adv.TriggerMovesInGroup(2);	//This will only trigger Node 3 (Group 2)

		\endcode

	**/
	virtual void TriggerMovesInGroup(size_t groupNumber) = 0;


	bool Supported();
													/** \cond INTERNAL_DOC **/
	// Construction
protected:
	IPortAdv(IPort &ourPort, IAttnPort &attnPort);
public:
	virtual ~IPortAdv() {}
													/** \endcond **/
};
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME																	      *
// 	IPort class
/**
	\brief Serial Port Interface.

	This class defines a "super" Port whose framework exposes the Port Level features
	available for your application by a series of public attribute references.
	There are common member functions for these feature objects to determine
	if the node supports the feature.

	This class also implements:
	- References to Node object on the Port
	- Functions to restart the Port, and determine if the Port is currently Open 
	- A function to issue a Port wide Nodestop to every Node on this Port
	- A function to record the command history of all communication on this Port

	\CODE_SAMPLE_HDR
	// Create instance of sFoundation
	SysManager myMgr;
	// Define a port SC4-HUB attached to
	myMgr.ComHubPort(0, 13);
	// Tell system to open this one port
	myMgr.PortsOpen(1);
	// Create useful reference to the first port object
	IPort &myPort = myMgr.Ports(0);
	// Test for presence of nodes
	if (myPort.NodeCount() == 0) {
	// Bail out, no nodes
		return;
	}
	\endcode

	\see sFnd::SysManager For the root object for interacting with
	ClearPath-SC motors.
	\see sFnd::SysManager::Ports To get references to the IPort objects created
	when SysManager::PortsOpen has successfully opened.
	**/
class MN_EXPORT IPort
{
													/** \cond INTERNAL_DOC **/
	friend class CPMportAdv;
public:
protected:
	/// Port index number, zero based.
	netaddr m_netNumber;
public:
													/** \endcond **/

public:
	/**
		\brief Return a reference to the indexed node on this port

		\param[in] index Get a reference for the node. This is the zero based
		index, meaning zero is the first node. The first node is the node
		closest to the host serial port in the ring.
	**/
	INode &Nodes(size_t index);

	/**
		\brief Return the count of nodes active on this port

		\return Return the count of nodes present on this port.

		Use this function to find out how many active nodes are attached to
		this port. If the port if offline, this will be zero.

		\CODE_SAMPLE_HDR
		// Check to see if there are nodes on the Port.
		if (myPort.NodeCount() == 0) {
		// Bail out, no nodes
		return;
		}
		\endcode
	**/
	virtual Uint16 NodeCount() = 0;

	/**
		\brief Restart this nodes to its powered on state and re-connect

		This causes the nodes on this port to restart and re-establish 
		contact with the detected node configuration.
	**/
	virtual void RestartCold() = 0;

	/**
		\brief Get port state

		\return Current port operational state.

		Use this function to ascertain the operation state of this port.

		\note Ports operate independently from each other. In other words, a
		failure on connections on one port should not affect another port.
	**/
	virtual openStates OpenState() = 0;

	/**
		\brief Wait for the port to go online

		\param[in] timeoutMsec Time to wait for port to return or go online

		\return True if the port came back online.

		This function is useful after restarting the network after power is lost.
	**/
	virtual bool WaitForOnline(int32_t timeoutMsec = 15000) = 0;

	/**
		\brief Create command trace file.

		This function creates a file which contains a detailed transaction log 
		of the all the commands	send and data received by the host. It will 
		remember the last 4096 commands and/or responses.

		Teknic support staff may ask for the file created via this function
		to determine where a programming error occurred.
	**/
	virtual void CommandTraceSave(const char *filePath) = 0;

	/**
		\brief Send a high-priority <i>Node Stop</i> command to all the nodes
		attached to this serial port. If stopType is
		STOP_TYPE_IGNORE, the nodes will use their default stop type instead 
		of the specific type. 

		\param [in] stopType Will force this stop type to all nodes on this port.

		This is a fast acting command that can be used to stop all motion on a
		port. If a modifier field is set, such as E-Stop, all motion will be 
		blocked until this condition is cleared.

		\see #sFnd::IMotion::NodeStop For initiating node stops at a single node.

		\CODE_SAMPLE_HDR

		myport.NodeStop(STOP_TYPE_ABRUPT);				//This issues an abrupt nodestop on all the nodes on this port
		myport.NodeStop(STOP_TYPE_RAMP_WITH_DECEL);		//This issues a nodestop to ramp to zero velocity using the current decel parameter's on all the nodes on this port

		\endcode
	**/
	virtual void NodeStop(mgNodeStopReg stopType = STOP_TYPE_IGNORE) = 0;

	/**
		\brief Group Shutdown Feature

	**/
	IGrpShutdown &GrpShutdown;

	/**
		\brief Brake Control Feature of SC-Hub
	**/
	IBrakeControl &BrakeControl;

	/**
		\brief Access to advanced features for the port.
	**/
	IPortAdv &Adv;

													/**	\cond INTERNAL_DOC	**/
	netaddr NetNumber() { return m_netNumber; }
	void NetNumber(netaddr index) { m_netNumber = index; }
													/** \endcond **/

public:
													/** \cond INTERNAL_DOC **/
	// Construction
	IPort(netaddr index, IBrakeControl &brake, IGrpShutdown &shtDwn,
		IPortAdv &advPort);
	virtual ~IPort();
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IMotionAudit class
																/** \cond SC_EXPERT **/
/**
	\brief Motion Audit Feature Interface

	This class object implements access to the motion audit feature. This
	feature is used to watch a reference move and determine if the axis
	has problems that may require service. 

	\note This is only available with Advanced feature set.
**/
class MN_EXPORT IMotionAudit : public IObjWithNode
{
private:
public:
	/**
		\brief Audit Monitoring Testpoints
	**/
	enum _testPoints {
		/**
		Watch Positional tracking errors.
		**/
		MON_POS_TRK = 6,
		/**
		Watch measured torque.
		**/
		MON_TRQ_MEAS = 7,
		/**
		Watch commanded torque.
		**/
		MON_TRQ_CMD = 8
	};
	/**
		\brief Result from last Refresh.

		After calling the \a Refresh function, the results are stored here.

		\note This feature is only available on Advanced nodes.
	**/
	mnAuditData Results;

	/**
		\brief Update the results.

		Calling this member function will retrieve the last collected audit
		information updating by calling IMotionAutit::Results function..

		\note This feature is only available on Advanced nodes.
	**/
	virtual void Refresh() = 0;

	/**
		\brief Setup monitoring test point

		\param[in] testPoint The node test point to monitor and collect
		statistics on.
		\param[in] fullScale The gain to apply for the selected channel. This
		should be carefully selected to match the expected values. If set too
		small, the values could be clipped in amplitude and if too large,
		quantization effects could affect the RMS and high frequency results.
		\param[in] filterTCmsec Low-pass filtering time constant specified
		in milliseconds.

		This function sets up the monitoring system test point. This test
		point collects high pass, low pass, maximum positive value, maximum
		negative value and RMS values during the collection interval.
	**/
	virtual void SelectTestPoint(enum _testPoints testPoint,
		double fullScale, double filterTCmsec) = 0;

	// Construction
	/** cond INTERNAL_DOC **/
protected:
	IMotionAudit(INode &ourNode);
public:
	virtual ~IMotionAudit() {}
	/** endcond **/
};
														/** \endcond  **/
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IAttnNode class
/**
	\brief <i>Node Attention</i> Feature Interface.

	<i>Node Attention</i> Feature Interface.

	A reference to this object allows your application to control which node
	status events will generate a <i>Network Attention</i> packet. These packets
	efficiently signal the host some noteworthy event, such as move done, has
	occurred. Using Attentions eliminates the overhead and latency involved with
	polling.

	\note This feature is only available on nodes that have the Advanced
	feature set.

	\see \ref AttentionsPage
	\see cpmStatusRegFlds for a complete list of Node status events

	
**/
class MN_EXPORT IAttnNode : public IObjWithNode
{
													/** \cond INTERNAL_DOC **/
	friend class INodeAdv;
	friend class INode;
													/** \endcond **/
protected:
public:
	/**
		\brief Attention Enabling Mask

		Setup which fields of the <i>Status Attn/Rise Register</i> cause the
		generation of an <i>Attention Packet</i> to the host. These packets are
		similar to a microprocessor interrupt.

		\see _cpmStatusRegFlds		For details about ClearPath-SC specific status
		\see _mnStatusReg		For details about the <i>Universal Status Register</i>

		\note This is an advanced feature.
	**/
	ValueStatus Mask;
												/** \cond INTERNAL_DOC **/
	/**
		\brief Warnings to indicate in the <i>Status Register</i>

		Setup which fields of the <i>Warning Register</i> will set the
		cpmStatusRegFlds::Warning field in the <i>Status Register</i>.

		This can be used to send attentions on drive events such as torque
		saturations.

		\see _cpmStatusRegFlds		For details about ClearPath-SC specific status
		\see _mnStatusReg		For details about the <i>Universal Status Register</i>
	**/
	ValueAlert WarnMask;
												/** \endcond  **/

	/**
		\brief Selection of Alerts to include in the <i>Status Register</i>
		User Alert field.

		Setup which fields of the <i>Alert Register</i> will set the
		cpmStatusRegFlds::UserAlert field in the <i>Status Register</i>.

		\see _cpmStatusReg		For details about ClearPath-SC specific status
		\see _mnStatusReg		For details about the <i>Universal Status Register</i>
	**/
	ValueAlert AlertMask;

	/**
		\brief Selection of Status Events to include in the <i>Status Register</i> Status Event field.

		Setup which fields of the <i>Status Register</i> will set the
		cpmStatusRegFlds::StatusEvent field in the <i>Status Register</i>.

		\note Do not include the \e StatusEvent field in this mask to avoid
		latching this field in the status register.

		\see _cpmStatusRegFlds		For details about ClearPath-SC specific status
		\see _mnStatusReg		For details about the <i>Universal Status Register</i>
	**/
	ValueStatus StatusMask;

	/**
		\brief Wait for any of the indicated attention fields to assert.

		\param[in] theAttn The set of attentions to wait on. \note Only the lower
		32-bits of the status register generate attentions.
		\param[in] timeoutMsec The maximum time to wait for these attentions.
		\param[in] autoClear If set, the bits underlying last attention register
		cooresponding to theAttn will be atomically cleared.
		\return A copy of the attentions that released this function. If the
		fields of this register is zero, the timeout occurred.

		When called, this function blocks until the desired attention occurs.
		The returned mnStatusReg is set with the fields of the last
		matching attention. This is helpful when waiting on multiple attentions
		to find out the actual attention signaled. The \a timeoutMS parameter
		allows you limit the amount of waiting time until the return. This is
		typically required to prevent your application from hanging up when
		something fails to deliver the expected event(s).

		\CODE_SAMPLE_HDR
		//Enable attentions on the Port (typically this would be enabled once ate the beginning of the application)
		myPort.Adv.Attn.Enable(true);

		// Wait for node to be ready to take commands
		attnReg attnReq;
		attnReq.cpm.Ready = 1;
		mynode.Adv.Attn.ClearAttn(attnReq);

		mynode.EnableReq(true);

		// If the node is not currently ready, wait for it to get there
		mynode.Status.RT.Refresh();
		if (!mynode.Status.RT.Value().cpm.Ready){
		attnReg theAttn = mynode.Adv.Attn.WaitForAttn(attnReq, 3000);
		if (!theAttn.cpm.Ready){
		mnErr eInfo;
		eInfo.ErrorCode = MN_ERR_TIMEOUT;
		eInfo.TheAddr = mynode.Info.Ex.Addr;
		snprintf(eInfo.ErrorMsg, sizeof(eInfo.ErrorMsg),
		"Error: Timed out waiting for enable\n");
		throw eInfo;
		}
		}
		\endcode
	**/
	mnStatusReg WaitForAttn(mnStatusReg theAttn, int32_t timeoutMsec,
						 bool autoClear = true);
	/**
		\brief Clear the indicated fields from the current attention state.

		\param[in] attnClr Set fields you wish to clear from the accumulating
		attentions.

		This member function is used to clear unwanted past Attentions that
		have been received by the host.
	**/
	void ClearAttn(mnStatusReg attnClr);

													/** \cond INTERNAL_DOC **/
	/**
		\brief Signal that an <i>Attention Packet</i> has arrived with the
		indicated attn fields set.

		\param[in] theAttn Detected attention information
	**/
	void SignalAttn(mnStatusReg theAttn);
	// Construction
protected:
	// Attention handling implementation.
	/**
		\brief Register that this thread will be looking for an attention.
	**/
	virtual void AttnRegister() = 0;
	/**
		\brief Unregister that this thread will no longer be looking for an 
		attention.
	**/
	virtual void AttnUnregister() = 0;
	/**
		\brief Wait for an attention to occur

		\param[in] timeoutMsec The maximum time to wait for an attention.
		\return True if the event has signaled before the timeout.

		\brief This member function wait up to \timeoutMsec for the attention to
		occur.
	**/
	virtual bool AttnWait(size_t timeoutMsec) = 0;
	/**
		\brief Signal that an attention occurred.
	**/
	virtual void AttnSignal() = 0;

	/**
		\brief Current attention state.
	**/
	mnStatusReg m_attn;

	IAttnNode(INode &ourNode,
		nodeparam attnMaskPnum, nodeparam usrWarnMaskPnum,
		nodeparam usrAlertMaskPnum, nodeparam usrStatMaskPnum);
															/** \endcond **/
public:
	virtual ~IAttnNode() {};
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IAttnPort class
													
/**
	\brief Serial Port Attention Feature Interface.

	This implements the feature object required to setup attentions and register 
	an Attention handler callback function on this port.

	\see AttentionsPage

**/
class MN_EXPORT IAttnPort : public IObjWithPort
{
													/** \cond INTERNAL_DOC **/
	friend class SysCPMattnPort;
private:
	bool AttnsEnabled;
	mnAttnCallback AttnCallback;
													/** \endcond **/
public:
													/** \cond SC_EXPERT **/
	/**
		\brief Return codes for IAttnPort::WaitForAttn.

		Use these return codes to interpret how the IAttnPort::WaitForAttn
		succeeded. Usually your application would perform some action
		when HAS_ATTN is returned.
	**/
	typedef enum _attnState {
		/**
		An attention was received.
		**/
		HAS_ATTN,
		/**
		No attentions have been received for a while.
		**/
		TIMEOUT,
		/**
		The attentions are disabled.
		**/
		DISABLED,
		/**
		The system is shutting down and no more attentions are going
		to be posted.
		**/
		TERMINATING
	} attnState;
													/** \endcond  **/
	/**
		\brief Control the posting of attentions 

		\param[in] newState If set, sFoundation will process received
		<i>Attention Requests</i>.

		\note This does not turn off the generation of attentions by the node.
		Turn off Attention generation by clearing the sFnd::IAttnNode::Mask for
		each node on the Port.  If attention posting is disabled, but attentions
		are still generated, an error will be generated.
	**/
	virtual void Enable(bool newState) = 0;

	/**
		\brief Return the global attention posting switch

		\return True if attentions can be posted.

	**/
	virtual bool Enabled() = 0;
													/** \cond SC_EXPERT **/
	/**
		\brief Wait for an attention to arrive. 
	
		This function will periodically
		return TIMEOUT to allow the application to terminate when no attentions
		have been detected.

		\param[out] attnRecvd If the function returns IAttnPort::HAS_ATTN, this
		buffer will be filled in with detected event.
		\return attnState To determine processing of the attn buffer.
	**/
	virtual attnState WaitForAttn(mnAttnReqReg &attnRecvd) = 0;
													/** \endcond  **/
	/**
		\brief Detect if handler is defined.

		\return true if a handler is installed.

		Use the IAttnPort::AttnHandler function to register a handler callback
		function.
	**/
	bool HasAttnHandler();

	/**
		\brief Register an attention callback function.

		Register an attention callback function.

		\param[in] theNewHandler The address of a handler function of type
		mnAttnCallback;

		\note To prevent system performance issues, the implemented callback
		function should not run for extended periods of time. The function is
		also restricted from running any network based command. This includes
		parameter accesses and node commands. This function should signal other threads to
		restart themselves or other lightweight signaling mechanisms.

		\CODE_SAMPLE_HDR
		// Example showing a Attention Handler function
		// Enable the attentions for this port
		myPort.Adv.Attn.Enable(true);
		// The attentions will be handled by the individual nodes, but register
		// a handler at the port level, just for illustrative purposes.
		myPort.Adv.Attn.AttnHandler(AttentionDetected);

		void MN_DECL AttentionDetected(const mnAttnReqReg &detected)
		{
		// Make a local, non-const copy for printing purposes
		mnAttnReqReg myAttns = detected;
		// Create a buffer to hold the attentionReg information
		char attnStringBuf[512];
		// Load the buffer with the string representation of the attention information
		myAttns.AttentionReg.StateStr(attnStringBuf, 512);
		// Print it out to the console
		printf("  --> ATTENTION: port %d, node=%d, attn=%s\n",
		detected.MultiAddr >> 4, detected.MultiAddr, attnStringBuf);
		}
		\endcode
	**/
	void AttnHandler(mnAttnCallback theNewHandler);
													/** \cond INTERNAL_DOC **/
	/**
		\brief Invoke the attention handler if defined.

		\param[in]  detected attention

		This is an internal function and should not be used.
	**/
	void InvokeAttnHandler(const mnAttnReqReg &detected);

	// Construction
protected:
	IAttnPort(IPort &ourPort);
	virtual ~IAttnPort();
													/** \endcond **/
};
													
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ILimitsAdv Class
													/** \cond SC_EXPERT **/
/**
	\brief Advanced Limit Feature Interface.

	This class object implements the settings for directional torque limiting.
**/
class MN_EXPORT ILimitsAdv : public IObjWithNode
{
													/** cond INTERNAL_DOC **/
	friend class CPMnode;
													/** endcond **/
public:
	/**
		\brief Access the node's positive torque limit object.

		Access the node's positive torque limit parameter object using the current torque 
		units. This parameter determines the positive torque limit used when positive torque limiting 
		is started.

		\see sFnd::INode::TrqUnit To determine the unit
		\see sFnd::INode::TrqUnit(_trqUnits) To change the unit
		\see sFnd::ValueDouble for manipulation examples
	**/
	ValueDouble PositiveTrq;

	/**
		\brief Access the node's positive torque limit time constant. 
	
		Access the node's positive torque limit time constant parameter object. The value is
		specified in milliseconds.  This parameter controls how quickly the torque limit is changed
		after positive torque limiting has started.  A longer time constant will produce a smoother 
		change in torque, a shorter time constant will produce a more sudden change.

		\see sFnd::ValueDouble for manipulation examples
	**/
	ValueDouble PositiveRelaxTCmsec;

	/**
		\brief Access the node's negative torque limit.

		Access the node's negative torque limit parameter object in the current torque units.  
		This parameter determines the negative torque limit used when negative torque limiting 
		is started.

		\see sFnd::INode::TrqUnit To determine the unit
		\see sFnd::INode::TrqUnit(_trqUnits) To change the unit
		\see sFnd::ValueDouble for manipulation examples
	**/
	ValueDouble NegativeTrq;

	/**
		\brief Access the node's negative torque limit time constant. 
		
		Access the node's negative torque limit time constant parameter object. The value is
		specified in milliseconds.  This parameter controls how quickly the torque limit is changed
		after negative torque limiting has started.  A longer time constant will produce a smoother 
		change in torque, a shorter time constant will produce a more sudden change.

		\see sFnd::ValueDouble for manipulation examples
	**/
	ValueDouble NegativeRelaxTCmsec;

	/**
		\brief Control the positive torque limit feature.

		Control user initiate the positive torque limit feature when 
		\a engage is true.  If \a engage is false positive torque limiting is
		turned off.
	
		\note If the node has been configured through Clearview to start limiting positive torque based off
		and input, or move done, etc. this request is ignored but remembered as the user's selection.
	**/
	virtual void StartPosFoldback(bool engage) = 0;

	/**
		\brief Get current state of the positive torque limit.

		Return the status of the user initiated positive torque limiting.  True indicates the positive 
		torque limit is active.  False indicates the positive torque limit is inactive. 

		\note  If the node has been configured through Clearview to start limiting positive torque based off
		and input, or move done, etc. this request returns the user's requested state, not the actual state.
	**/
	virtual bool StartPosFoldback() = 0;

	/**
		\brief Control the negative torque limit feature.

		Control user initiate the negative torque limit feature when
		\a engage is true.

		\note If the node has been configured through Clearview to start limiting negative torque based off
		and input, or move done, etc. this request is ignored but remembered as the user's selection.
	**/
	virtual void StartNegFoldback(bool engage) = 0;

	/**
		\brief Get current state of the negative torque limit.

		Return the status of the user initiated negative torque limiting.  True indicates the negative 
		torque limit is active.  False indicates the negative torque limit is inactive. 

		\note If the node has been configured through Clearview to start limiting negative torque based off
		and input, or move done, etc. this request returns the user's requested state, not the actual state.
	**/
	virtual bool StartNegFoldback() = 0;

protected:
													/** cond INTERNAL_DOC **/
	// Construction 
	ILimitsAdv(INode &ourNode,
		nodeparam posFldkPnum, nodeparam posFldkTCpNum,
		nodeparam negFldkPnum, nodeparam negFldkTCpNum);
public:
	virtual ~ILimitsAdv() {}
													/** endcond **/
};
													/** \endcond  **/
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	INodeAdv class
/**
	\brief Advanced Node Features Interface.

	This implements the objects required to access any advanced features
	of a node.

	Provides access to the following advanced features:
	- Attention Generation
**/
class MN_EXPORT INodeAdv : public IObjWithNode
{
public:
	/**
		\brief Attention Generation Feature Setup.

		Access the node's <i>Attention Generation</i> feature.

	**/
	IAttnNode &Attn;
											/** \cond SC_EXPERT **/
	/**
		\brief Motion Audit Feature Access

		This feature can be used to monitor an axis' operational 
		characteristics and monitor for changes.
	**/
	IMotionAudit &MotionAudit;
											/** \endcond  **/
													/** \cond INTERNAL_DOC **/
	// Construction
protected:
	INodeAdv(INode &ourNode, IMotionAudit &audit, IAttnNode &attnNode);
	virtual ~INodeAdv() {};
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	INodeEx class
/** \cond SC_EXPERT **/
/**
	\brief Expert Node Features Interface.

	This implements the objects required to access any expert features
	of a node.
**/
class MN_EXPORT INodeEx : public IObjWithNode
{
													/** cond INTERNAL_DOC **/
	friend class INode;
													/** endcond **/
													/** cond INTERNAL_DOC **/
protected:
	// Read-modify-write parameter mutex access via INode implementations
	// using the INode::UseMutex class.
	/**
		\brief Initiate an update lock access to registers

		If directly modifying a register for read-modify-write, you should
		take this lock before performing read-modify-write operations. This
		prevents value corruption by other threads trying to perform similar
		operations.
	**/
	virtual void MutexTake() = 0;
	/**
		\brief Release the update access lock for read-modify-write
		operation

		After completing a read-modify-write operation you need to release
		the lock to allow other threads to have access.

		\note Failure to release the mutex will lead to system dead-lock. Insure
		all error paths release this lock as well.
	**/
	virtual void MutexRelease() = 0;
	// Construction
													/** endcond **/
protected:
													/** cond INTERNAL_DOC **/
	INodeEx(INode &ourNode);
public:
	virtual ~INodeEx() {}
													/** endcond **/
};		
/** \endcond  **/
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	IHoming class
/**
	\brief Homing Feature Interface.

	This class object implements access to Teknic's homing engine.  This object allows
	a user to initiate homing and test whether homing was complete. This object
	cannot configure homing parameters such as homing direction/speed.  Homing parameters
	must be set using the ClearView software, and this object only provides the ability
	to start homing and check whether the axis has homed.

	Homing is required to use soft limits and
	establish the node's number space for absolute type moves.
**/
class MN_EXPORT IHoming : public IObjWithNode
{
													/** \cond INTERNAL_DOC **/
	friend class INode;
													/** \endcond **/

public:
	/**
		\brief Initiate the homing sequence.

		This function is called to initiate a homing sequence. ClearView
		should be used to setup how homing is performed. 

		\CODE_SAMPLE_HDR
		// Initiate Homing
		mynode.Motion.Homing.Initiate();
		\endcode
	**/
	virtual void Initiate() = 0;

	/**
		\brief Update status real-time and return the state of the homing active
		flag.

		Update [status real-time register](@ref sFnd::IStatus::RT) and return the state of the homing active
		flag.

		\return True if homing is occurring.

		\CODE_SAMPLE_HDR
		// Check to see if the Node is still homing
		if(mynode.Motion.Homing.IsHoming())
		{ StillHoming }
		else
		{ No Longer Homing  }

		\endcode

	**/
	virtual bool IsHoming() = 0;

	/**
		\brief Update status real-time and return the state of the homing 
		complete flag.

		Update [status real-time register](@ref sFnd::IStatus::RT) and return the state of the homing
		complete flag.  Homing complete will only assert after a successful homing sequence
		or if the user calls the SignalComplete() function indicating a successful manual homing.

		\return True if homing has completed.

		\CODE_SAMPLE_HDR
		// Check to see if the Node is still homing
		if(mynode.Motion.Homing.WasHomed())
		{ Homed Successfully }
		else
		{ Not Successfuly Homed  }

		\endcode
	**/
	virtual bool WasHomed() = 0;

	/**
		\brief Signal the completion of manual homing.

		This function is used to signal to the node that the homing sequence
		has completed. Typically the homing process automatically 
		proceeds until completion. If a hard-stop or input cannot
		be used to terminate the homing sequence, this function can be
		used to signal the completion of a homing sequence.

		Calling this function will cause soft limits to activate.

		\note This function is used when the  automatic homing
		feature cannot be used to establish the absolute number space. For
		example when camera processing is used to establish a position.

		\see IHoming::SignalInvalid To disable soft limits and allow another
		homing sequence to occur.
		\see IMotion::AddToPosition to manually adjust the number space during manual homing

		\CODE_SAMPLE_HDR

		// Manually zero position, and declare the number space valid
		mynode.Motion.AddToPosition(-mynode.Motion.MeasPosition);
		mynode.Motion.Homing.SignalComplete();

		\endcode

	**/
	virtual void SignalComplete() = 0;

	/**
		\brief Signal the node that our absolute position space is invalid. 
		
		Signal the node that our absolute position space is invalid.

		This is	useful when using soft limits and manual homing procedure.

		\note After calling this function the soft limit feature, if enabled,
		is disabled.

		\see IHoming::SignalComplete to reverse the affects of this function.

		\see IMotion::AddToPosition to manually adjust the number space during manual homing
	**/
	virtual void SignalInvalid() = 0;

	/**
		\brief Check Homing settings to see if they are set-up properly

		\return True if homing has been set-up
	**/
	virtual bool HomingValid() = 0;

protected:
	// Construction
													/** \cond INTERNAL_DOC **/
	IHoming(INode &ourNode, nodeparam accPnum, nodeparam velLimPnum,
		nodeparam clampPnum, nodeparam offsPnum, nodeparam postMvDlyPnum);
public:
	virtual ~IHoming() {}
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IInfoAdv class

													/** \cond SC_EXPERT **/
/**
	\brief Advanced Information Features Interface.

	This class object implements access to the advanced node information.

	There is currently no advanced information available.
**/
class MN_EXPORT IInfoAdv : public IObjWithNode
{
private:
public:
protected:
													/** cond INTERNAL_DOC **/
	// Constructor
	IInfoAdv(INode &ourNode);
													/** endcond **/
};
													/** \endcond  **/
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IInfoEx class
													
/**
	\brief Expert Information Interface.

	This class object implements access to expert node information.
**/
class MN_EXPORT IInfoEx : public IObjWithNode
{
													/** \cond INTERNAL_DOC **/
private:
	multiaddr m_addr;
protected:
	void Addr(multiaddr assignAddr);
													/** \endcond **/
public:
	/**
		\brief Get our internal network address.

		\return The network address for this node.

		This information allows the retrieval of the internal network address.
	**/
	multiaddr Addr();

	/**
		\brief Get this node's network location

		\return The node index for this port.

		This is the zero based index of the location of the node. The first
		node from the host starts the count.
	**/
	nodeaddr NodeIndex();

	/**
		\brief Get a numeric parameter's current value

		\param[in] index The parameter number for a numeric parameter.
		\return Numeric value. The scaling and units depend on the requested
		parameter.

		\note This function should only be used under direction of Teknic support
		staff.
	**/

	virtual double Parameter(nodeparam index) = 0;
	/**
		\brief Set a numeric parameter's new value

		\param[in] index The parameter number for a numeric parameter.
		\param[in] newValue The new value to write into this parameter.

		\note This function should only be used under direction of support
		staff.
	**/
	virtual void Parameter(nodeparam index, double newValue) = 0;
													/** \cond INTERNAL_DOC **/
protected:
	IInfoEx(INode &ourNode);
public:
	virtual ~IInfoEx() {}
													/** \endcond **/
};

//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IInfo class
/**
	\brief Node Information Interface.

	This class object implements access to node information.

	This includes node management items such as:
	- Serial Number
	- Node Type
	- Firmware Version
	- User ID management
	- Configuration file loading and saving
**/
class MN_EXPORT IInfo : public IObjWithNode
{
private:
public:
	/// This enumeration describes the type of node this object represents.
	typedef enum _nodeTypes {
		/// The type cannot be determined
		UNKNOWN,
								/** \cond SC_EXPERT **/
		/// The node is a Meridian ISC
		MERIDIAN_ISC,
								/** \endcond  **/
		/// The node is a ClearPath-SC
		CLEARPATH_SC,
		/// The node is an Advanced ClearPath-SC
		CLEARPATH_SC_ADV
	} nodeTypes;

	/**
		\brief Get serial number of this node.

		Access the node serial number via the Value function.

		\CODE_SAMPLE_HDR
		printf( "SN=%u\n", uint32_t(myNode.Info.SerialNumber));
		\endcode
	**/
	ValueUnsigned SerialNumber;

	/**
		\brief Return a firmware version string.
		
		The string is in the format of "x.y.z checksum" for example, "1.0.2 E113".

		\CODE_SAMPLE_HDR
		printf( "FW Ver=%s\n", myNode.Info.FirmwareVersion.Value());
		// prints: 1.0.2 E113
		\endcode
	**/
	ValueString &FirmwareVersion;
														/** \cond SC_EXPERT **/
	/**
		\brief Get the version number code.

		This is an unique value where the magnitude increases as newer firmware
		is released.

		\CODE_SAMPLE_HDR
		printf( "FW code=0x%x\n", uint32_t(myNode.Info.FirmwareVersionCode));
		// prints: 0x1002 which represents 1.0.2
		\endcode
	**/
	ValueUnsigned FirmwareVersionCode;

	/**
		\brief Get the hardware version code string.

		This reference allows access to the node's hardware version string.

		\CODE_SAMPLE_HDR
		printf( "HW Ver=%s\n", myNode.Info.HardwareVersion.Value());
		// prints: K1
		\endcode
	**/
	ValueString &HardwareVersion;
															/** \endcond  **/
	/**
		\brief Get the model string.

		This reference allows access to the model string for this node

		\CODE_SAMPLE_HDR
		printf( "Model=%s\n", myNode.Info.Model.Value());
		// prints: CPM-SCSK-3411P-ELSA-1-0-D
		\endcode
	**/
	ValueString &Model;

	/**
		\brief Get the positioning resolution

		This function allows access to the node's positioning resolution. 
		The returned value is the number of position counts per revolution.

		\CODE_SAMPLE_HDR
		//Get the positioning resolution of the Node
		uint32_t myPosnResolution = myNode.Info.PositioningResolution;
		\endcode
	**/
	ValueUnsigned PositioningResolution;

	/**
		\brief Return the enumerated type of this node.

		This function is used to determine the type and base capabilities 
		of this node. 

		\CODE_SAMPLE_HDR
		if (myNode.Info.NodeType() == IInfo::CLEARPATH_SC) {
			printf( "My Node is a ClearPath-SC\n");
			return(2);
		}
		if(myNode.Info.NodeType() != IInfo::CLEARPATH_SC_ADV){
			printf( "My Node is an Advanced ClearPath-SC\n");
			return(2);
		}
		\endcode
	**/
	virtual nodeTypes NodeType() = 0;

	/**
		\brief Manage the UserID string for this node.

		Manage the UserID string for this node.  

		\note This parameter is saved to and overwritten by [loading configuration files](@ref configfileLoadPage), or resetting to factory defaults

		\CODE_SAMPLE_HDR
		// Set User ID via the Value function
		myNode.Info.UserID.Value("My Node");
		// Set User ID via the operator= override
		myNode.Info.UserID = "My Node";
		// Get User ID via the Value function
		cout << myNode.Info.UserID.Value() << endl;
		// Get User ID via operator const char*()
		const char *myParam = myNode.Info.UserID;
		\endcode
	**/
	ValueString &UserID;

	/**
		\brief Set the Current User Defined Non-volatile Data.

		\param [in] bank which bank of data to write to [0-3].

		\param [in] m_lastUserData The data to write into the Node.  MN_USER_NV_SIZE = 13 for ClearPath SC Nodes.


		This feature allows the storage of calibration or other data at a node
		location. This information will be retained without power and survives
		node restarts, and configuration file changes.  Up to 4 banks of 13 bytes are available

		\note Restoring the node to Factory Settings will clear this 
		data.

		\CODE_SAMPLE_HDR
		// Data to write
		uint8_t newNV[MN_USER_NV_SIZE] = {
			0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x88,
			0x99, 0xaa, 0xbb, 0xcc, 0xdd
		};
		// Write new data to bank 0
		myNode.Info.UserData(0, newNV);
		\endcode
	**/
	virtual void UserData(size_t bank, 
						  const uint8_t m_lastUserData[MN_USER_NV_SIZE]) = 0;

	/**
		\brief Get the Current User Defined Non-volatile Data.

		\param [in] bank which bank of data to access [0-3].

		This function allows access to the non-volatile storage areas.

		\note Restoring the node to Factory Settings will clear this 
		data.

		\CODE_SAMPLE_HDR
		// Get non-volatile data from node bank 0.
		vector<uint8_t> myUserNV = myNode.Info.UserData(0); 
		\endcode
	**/
	virtual std::vector<uint8_t> UserData(size_t bank) = 0;


	/**
		\brief Manage User Volatile Data.

		This feature is useful for your application to stash information in
		a node in case the host application crashes.

		\note This value is set to zero up power-up or restarting.

		\CODE_SAMPLE_HDR
		// Set the volatile value
		myNode.Info.UserRAM = 0x1234
		// Get the volatile value
		cout << myNode.Info.UserRAM.Value() << endl;
		// Alternate access way
		uint16_t myVolatile = myNode.Info.UserRAM;
		\endcode
	**/
	ValueUnsigned UserRAM;
													
	/**
		\brief Expert Information Access.

		This object allow access to expert information.

		These include:
		- The network address
		- The node index for this node
		- Generic Parameter maintenance
	**/
	IInfoEx &Ex;
													/** \cond SC_EXPERT **/
	/**
		\brief Advanced model information access.

		This object will work with Advanced firmware nodes.

		There is no feature currently implemented here yet.
	**/
	IInfoAdv &Adv;
													/** \endcond  **/
protected:
													/** \cond INTERNAL_DOC **/
	// Constructor
	IInfo(INode &ourNode, IInfoAdv &adv, IInfoEx &ex,
		nodeparam snPnum, nodeparam fwPnum, nodeparam usrRamPnum,
		ValueString &userID, ValueString &hwVer, ValueString &fwStr,
		ValueString &model, nodeparam resPnum);
public:
	virtual ~IInfo() {}
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ILimits Class
											
/**
	\brief Torque and Position Limits Feature Interface.

	This class object implements the settings for torque limits and
	positional limits.  
**/
class MN_EXPORT ILimits : public IObjWithNode
{
public:
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Torque Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Access the node's global torque limit.

		Access the node's global torque limit in the current user [torque units.](@ref sFnd::INode::TrqUnit).  
		Typically this value is set through the ClearView application and is not changed during the application.

		\CODE_SAMPLE_HDR
		// Set the torque Limit to 100%
		myNode.TrqUnit(myNode.PCT_MAX);
		myNode.Limits.TrqGlobal = 100;
		// Print out the torque limit
		cout << myNode.Limits.TrqGlobal.Value() << endl;
		// Alternate access way
		double myTrqLimit = myNode.Limits.TrqGlobal;
		\endcode
	**/
	ValueDouble TrqGlobal;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Position Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
																	/** \cond SC_EXPERT **/
	/**
		\brief Access the software limits 1.

		Access the software limits 1.  Typically these values are set through the ClearView application and are not changed during the application.

		\note Soft limits are only enabled when the absolute number space has been established via [homing](@ref HomingPage)
	**/
	ValueSigned SoftLimit1;

	/**
		\brief Access the software limits 2.

		Access the software limits 2. Typically these values are set through the ClearView application and are not changed during the application.

		\note Soft limits are only enabled when the absolute number space has been established via [homing](@ref HomingPage)
		**/
	ValueSigned SoftLimit2;
																	/** \endcond  **/
	/**
		\brief Access the tracking limit shutdown point.
		
		Access the tracking limit shutdown point. This parameter is a safety limit should not be changed by the application 
		during operation except is rare application specific circumstances.
		
		[Tracking error](@ref sFnd::IMotion::PosnTracking) at or beyond this point will cause a safety shutdown to occur. 
	**/
	ValueUnsigned PosnTrackingLimit;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Velocity Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
																	/** \cond SC_EXPERT **/
	/**
		\brief Access the motor speed limit. 
		
		This value is returned in the current user velocity units.
		\todo Would this ever need to be changed or can we hide this?
	**/
	ValueDouble MotorSpeedLimit;
	/**
		\brief Access the advanced limits object.

		Access to the advanced limit features for Advanced ClearPath-SC Nodes.
	**/
	ILimitsAdv &Adv;
																	/** \endcond  **/

protected:
													/** \cond INTERNAL_DOC **/
	// Construction 
	ILimits(INode &ourNode, ILimitsAdv &adv, 
		nodeparam globTrqPnum, nodeparam trkLimPnum,
		nodeparam softLim1Pnum, nodeparam softLim2Pnum,
		nodeparam mtrSpdLimPnum);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IMotionAdv Class
/**
	\brief Advanced Motion Control Interface.

	This class object allows setup and initiation of advanced motion features only available
	in advance models of Clearpath SC.  These features include:

	- [Head, and Tail Moves](@ref HeadTailMoveSect)
	- [Asymmetric moves](@ref AsymetricalMoveSect)
	- [Triggered Moves](@ref	CPMtriggerPage)
	- [A after start](@ref cpmStatusRegFlds::AFromStart)/ [B after end events](@ref cpmStatusRegFlds::BFromEnd)  

	\see \ref CPMAdvancedMovePage for more information.


**/
class MN_EXPORT IMotionAdv : public IObjWithNode
{
public:
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Kinematic Constraint Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Access the deceleration limit for the next issued Asymmetric move. 
		
		Access the decceleration limit value for the next asymmetric move. The value
		is specified using this node's user acceleration units. Changed values will
		apply to the next move.  The default units are RPM/sec
	
		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the Decceleration Limit
		myNode.Motion.Adv.DecelLimit = 10000.;				// Sets
		double accLim = myNode.Motion.Adv.DecelLimit;	// Gets
		\endcode

		\see INode::AccUnit(enum _accUnits newUnits) Changing Acceleration Units
	**/
	ValueDouble DecelLimit;

	/**
			\brief Access the velocity limit for the head-tail sections of the
		next issued head-tail move. 
	
		Access the current head/tail velocity limit for the next head-tail move.
		The value is specified in the current velocity
		units. Changed values will apply to the next issued move.  The default units 
		are RPM.

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the Head/Tail Velocity Limit
		myNode.Motion.Adv.HeadTailVelLimit = 1000.;				// Sets
		double velLim = myNode.Motion.Adv.HeadTailVelLimit;	// Gets
		\endcode

		\see INode::VelUnit(enum _velUnits newUnits)
	**/
	ValueDouble HeadTailVelLimit;

	/**
		\brief Access the head distance for the next issued head-tail move.

		Access the head distance for the next issued head-tail move
		in steps.

		\note The resolution of this value will be the same as the positioning resolution of the motor (800cnts/rev for "Regular" 6400cnts/rev for "Enhanced").

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the Head Distance 
		myNode.Motion.Adv.HeadDistance = 10000.;				// Sets
		double headDist = myNode.Motion.Adv.HeadDistance;	// Gets
		\endcode
		
	**/
	ValueUnsigned HeadDistance;

	/**
		\brief Access the tail distance for the next issued head-tail move.

		Access the tail distance for the next issued head-tail
		move in steps.

		\note The resolution of this value will be the same as the positioning resolution of the motor (800cnts/rev for "Regular" 6400cnts/rev for "Enhanced").

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the Tail Distance 
		myNode.Motion.Adv.TailDistance = 10000.;				// Sets
		double tailDist = myNode.Motion.AdvTaildDistance;	// Gets
		\endcode
	**/
	ValueUnsigned TailDistance;

	/**
		\brief Access the <i>A after Start</i> event distance.

		Access the <i>A after Start</i> event distance

		\note The resolution of this value will be the same as the positioning resolution of the motor (800cnts/rev for "Regular" 6400cnts/rev for "Enhanced").

		\see cpmStatusRegFlds::AFromStart for the associated Status event

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the A after start distance
		myNode.Motion.Adv.AfterStartDistance = 10000.;				// Sets
		double Adist = myNode.Motion.Adv.AfterStartDistance;	// Gets
		\endcode
	**/
	ValueUnsigned AfterStartDistance;
	/**
		\brief Access the <i>B before End</i> event distance.

		Access the <i>B before End</i> event distance in steps.

		\note The resolution of this value will be the same as the positioning resolution of the motor (800cnts/rev for "Regular" 6400cnts/rev for "Enhanced").

		\see cpmStatusRegFlds::BFromEnd for the associated Status event

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the B before end distance
		myNode.Motion.Adv.BeforeEndDistance = 10000.;				// Sets
		double Bdist = myNode.Motion.Adv.BeforeEndDistance;	// Gets
		\endcode
	**/
	ValueUnsigned BeforeEndDistance;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Motion Duration Calculation Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Calculate the duration of a positional move.

		\param [in] targetPosn Target position. This is either the
		absolute position of relative to the current position
		depending on the \a targetIsAbsolute argument.
		\param [in] targetIsAbsolute the target is an absolute
		destination.
		\return duration (milliseconds)

		Using the currently specified kinematic constraints, return the
		expected duration (in milliseconds) of the move from when it
		starts to when the move command completes.

		This function is useful for setting up timeouts or helping to schedule
		other events in your application.

		\CODE_SAMPLE_HDR
		// SAMPLE: Using MovePosnDurationMsec to set up a reasonable timeout for a move to complete

		//define a timeout in case the node is unable to complete the move
		double timeout =  mynode.Motion.Adv.MoveDurationMsec(1000) + 100;  
		double StartTime = myMgr.TimeStampMsec();

		//This will loop checking to see if the node went Move done with the defined timeout
		while (!mynode.Motion.MoveWentDone() && myMgr.TimeStampMsec() < StartTime+timeout) 
		{//Wait for move to complete
		}

		\endcode

	**/
	double MovePosnDurationMsec(int32_t targetPosn,
		bool targetIsAbsolute = false);

	/**
		\brief Returns the duration of a head-tail positional move.

		\param [in] targetPosn Target position. This is either the
		absolute position of relative to the current position
		depending on the \a theType argument. The triggered
		type move will prevent execution of the motion until a
		trigger event is detected at the node.
		\param [in] targetIsAbsolute the target is an absolute
		destination.
		\param [in] hasHead Do not exceed the HeadTailVelLimit until
		after HeadDistance steps have been sent.
		\param [in] hasTail Do not exceed the HeadTailVelLimit during
		the last TailDistance steps of the move.
		\return motion duration (milliseconds)

		Using the currently specified kinematic constraints, return the
		expected duration (in milliseconds) of the move from when it
		starts to when the move command completes.

		This function is useful for setting up timeouts or helping to schedule
		other events in your application.

		\CODE_SAMPLE_HDR
		// SAMPLE: Using MovePosnHeadTailDurationMsec to set up a reasonable timeout for a head-tail move to complete

		//define a timeout in case the node is unable to complete the move
		double timeout =  mynode.Motion.Adv.MovePosnHeadTailDurationMsec(1000) + 100;  
		double StartTime = myMgr.TimeStampMsec();

		//This will loop checking to see if the node went Move done with the defined timeout
		while (!mynode.Motion.MoveWentDone() && myMgr.TimeStampMsec() < StartTime+timeout) 
		{//Wait for move to complete
		}

		\endcode
	**/
	virtual double MovePosnHeadTailDurationMsec(int32_t targetPosn,
		bool targetIsAbsolute = false,
		bool hasHead = true,
		bool hasTail = true);

	/**
		\brief Returns the duration of an asymmetric positional move.

		\param [in] targetPosn Target position. This is either the
		absolute position of relative to the current position
		depending on the \a theType argument. The triggered
		type move will prevent execution of the motion until a
		trigger event is detected at the node.
		\param [in] targetIsAbsolute the target is an absolute
		destination.
		\return motion duration (milliseconds)

		Using the currently specified kinematic constraints, return the
		expected duration (in milliseconds) of the move from when it
		starts to when the move command completes.

		This function is useful for setting up timeouts or helping to schedule
		other events in your application.

		\CODE_SAMPLE_HDR
		// SAMPLE: Using MovePosnAsymDurationMsec to set up a reasonable timeout for an asymmetric move to complete

		//define a timeout in case the node is unable to complete the move
		double timeout =  mynode.Motion.Adv.MovePosnAsymDurationMsec(1000) + 100;  
		double StartTime = myMgr.TimeStampMsec();

		//This will loop checking to see if the node went Move done with the defined timeout
		while (!mynode.Motion.MoveWentDone() && myMgr.TimeStampMsec() < StartTime+timeout) 
		{//Wait for move to complete
		}

		\endcode
	**/
	virtual double MovePosnAsymDurationMsec(int32_t targetPosn,
		bool targetIsAbsolute = false);


	/**
		\brief Returns the duration of a velocity move.

		\param [in] targetCountsPerSec Target velocity. (counts/sec)		
		\return time to reach target velocity (milliseconds)

		Using the currently specified kinematic constraints, return the
		expected duration (in milliseconds) of the move's acceleration to the target velocity.

		This function is useful for setting up timeouts or helping to schedule
		other events in your application.

		\CODE_SAMPLE_HDR
		// SAMPLE: Using MoveVelDurationMsec to set up a reasonable timeout for a velocity move to complete

		//define a timeout in case the node is unable to complete the move
		double timeout = mynode.Motion.MoveVelDurationMsec() + 100;
		double StartTime = myMgr.TimeStampMsec();

		//This will loop checking to see if the node went Move done with the defined timeout
		while (!mynode.Motion.VelocityReachedTarget() && myMgr.TimeStampMsec() < StartTime+timeout)
		{//Wait for move to complete
		}

		\endcode
	**/
	double MoveVelDurationMsec(double targetCountsPerSec);


	/**/
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Motion Initiating Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Initiate a trapezoid/triangle positional move.

		\param [in] targetPosn Target position. This is either the
		absolute position of relative to the current position
		depending on the \a theType argument. The triggered
		type move will prevent execution of the motion until a
		trigger event is detected at the node.
		\param [in] targetIsAbsolute the target is an absolute
		destination.
		\param [in] isTriggered The move awaits a trigger event
		to start the motion if true.
		\param [in] hasDwell The move will wait the dwell parameter
		amount of time before starting the next move.
		\return Number of additional moves the node will accept.

		The move is kinematically limited by the current
		[acceleration](@ref sFnd::IMotion::AccLimit), and [velocity](@ref sFnd::IMotion::VelLimit) limits.

		Move requests are buffered and requests initiated during an active move will execute
		immediately after the active move completes.  Triggered moves will wait for a trigger event before executing.
		The return value will tell you the number of additional moves that may be queued(up to 16) when
		this request is processed.  You can also use the <i>Status
		Register's</i> #_cpmStatusRegFlds::MoveBufAvail field, which will be
		deasserted when the buffer is full.

		\see sFnd::IPortAdv::TriggerMovesInGroup For starting these moves
		\see sFnd::IMotionAdv::TriggerMove For starting these moves
	**/
	virtual size_t MovePosnStart(int32_t targetPosn,
		bool targetIsAbsolute = false,
		bool isTriggered = false,
		bool hasDwell = false) = 0;

	/**
		\brief Initiate a head-tail positional move.

		\param [in] targetPosn The target position. This is either the
		absolute position of relative to the current position
		depending on the \a theType argument. The triggered
		type move will prevent execution of the motion until a
		trigger event is detected at the node.
		\param [in] targetIsAbsolute the target is an absolute
		destination.
		\param [in] isTriggered The move awaits a trigger event
		to start the motion if true.
		\param [in] hasHead Do not exceed the HeadTailVelLimit until
		after HeadDistance steps have been sent.
		\param [in] hasTail Do not exceed the HeadTailVelLimit during
		the last TailDistance steps of the move.
		\param [in] hasDwell delay the next move being issued
		by DwellMs after this move profile completes.
		\return Number of additional moves the node will accept.

		The move is kinematically limited by the current
		[acceleration](@ref sFnd::IMotion::AccLimit), [velocity](@ref sFnd::IMotion::VelLimit), as well as [Head](@ref sFnd::IMotionAdv::HeadDistance) and [tail](@ref sFnd::IMotionAdv::TailDistance) distances.

		Move requests are buffered and requests initiated during an active move will execute
		immediately after the active move completes.  Triggered moves will wait for a trigger event before executing.
		The return value will tell you the number of additional moves that may be queued(up to 16) when
		this request is processed.  You can also use the <i>Status
		Register's</i> #_cpmStatusRegFlds::MoveBufAvail field, which will be
		deasserted when the buffer is full.

		\see sFnd::IPortAdv::TriggerMovesInGroup For starting these moves
		\see sFnd::IMotionAdv::TriggerMove For starting these moves
	**/
	virtual size_t MovePosnHeadTailStart(int32_t targetPosn,
		bool targetIsAbsolute = false,
		bool isTriggered = false,
		bool hasHead = true,
		bool hasTail = true,
		bool hasDwell = false) = 0;

	/**
		\brief Initiate a asymmetric positional move.

		\param [in] targetPosn Target position. This is either the
		absolute position of relative to the current position
		depending on the \a theType argument. The triggered
		type move will prevent execution of the motion until a
		trigger event is detected at the node.
		\param [in] targetIsAbsolute the target is an absolute
		destination.
		\param [in] isTriggered The move awaits a trigger event
		to start the motion if true.
		\param [in] hasDwell The move will wait the dwell parameter
		amount of time before starting the next move.
		\return Number of additional moves the node will accept.

		The move is kinematically limited by the current
		[acceleration](@ref sFnd::IMotion::AccLimit), [deceleration](@ref sFnd::IMotionAdv::DecelLimit), and [velocity](@ref sFnd::IMotion::VelLimit).

		Move requests are buffered and requests initiated during an active move will execute
		immediately after the active move completes.  Triggered moves will wait for a trigger event before executing.
		The return value will tell you the number of additional moves that may be queued(up to 16) when
		this request is processed.  You can also use the <i>Status
		Register's</i> #_cpmStatusRegFlds::MoveBufAvail field, which will be
		deasserted when the buffer is full.

		\see sFnd::IPortAdv::TriggerMovesInGroup For starting these moves
		\see sFnd::IMotionAdv::TriggerMove For starting these moves
	**/
	virtual size_t MovePosnAsymStart(int32_t targetPosn,
		bool targetIsAbsolute = false,
		bool isTriggered = false,
		bool hasDwell = false) = 0;

	/**
		\brief Initiate a velocity move.

		\param [in] targetPosn Target velocity. If the \a triggered
		argument is true, this move will wait for a trigger event
		to be detected at the node.
		\param [in] isTriggered The move awaits a trigger event
		to start the motion if true.
		\return Number of additional moves the node will accept.

		The move is kinematically limited by the current
		[acceleration limit](@ref sFnd::IMotion::AccLimit).

		Move requests are buffered and requests initiated during an active move will execute
		immediately after the active move completes.  Triggered moves will wait for a trigger event before executing.
		The return value will tell you the number of additional moves that may be queued(up to 16) when
		this request is processed.  You can also use the <i>Status
		Register's</i> #_cpmStatusRegFlds::MoveBufAvail field, which will be
		deasserted when the buffer is full.

	**/
	virtual size_t MoveVelStart(double targetPosn,
		bool isTriggered = false) = 0;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Triggering waiting moves
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	/**
		\brief Trigger a waiting move

		Call this function to release a waiting move.

		\note If there are no waiting moves or if the Node is currently executing a move, this command will be ignored.

		\CODE_SAMPLE_HDR
		// SAMPLE: This loads a relative triggered move into the move buffer
		mynode.Motion.Adv.MovePosnStart(1000,false,true);

		//No motion has occurred yet...

		mynode.Motion.Adv.TriggerMove();
		//Motion now begins

		\endcode
	**/
	virtual void TriggerMove() = 0;

	/**
		\brief Trigger a group of waiting moves

		Call this function to release waiting moves in this trigger group.

		\note If there are no waiting moves, or if the node is currently executing a move this command will be ignored on that node.

		/see CPMtriggerPage

		\CODE_SAMPLE_HDR
		// SAMPLE: This loads a relative triggered move into each node
		mynode.Motion.Adv.MovePosnStart(1000,false,true);
		myothernode.Motion.Adv.MovePosnStart(1000,false,true);

		//No motion has occurred yet...

		mynode.Motion.Adv.TriggerMovesInMyGroup();
		//If both Nodes share a trigger group, then both node will start motion now
		//If the nodes do not share a trigger group only mynode will begin motion

		\endcode
	**/
	virtual void TriggerMovesInMyGroup() = 0;

	/**
		\brief Get our current trigger group setting. 

		\return This node's current group number

		If zero, there is no trigger group assigned and the group trigger
		will be ignored.
	**/
	virtual size_t TriggerGroup() = 0;

	/**
		\brief Set our trigger group

		\param[in] groupNumber Set to non-zero to assign this
		node to a triggerable group. To remove from a group set to zero.

		\remark The trigger group allows multiple nodes to start motion with low
		latency. For example a machine with multiple XYZ axes could assign
		\a groupNumber 1 to one set and 2 to another. Releasing one group with
		a sFnd::IPortAdv::TriggerMovesInGroup(size_t) call, or by calling #TriggerMovesInMyGroup on one of the Nodes.

		\CODE_SAMPLE_HDR
		// SAMPLE: This will set two Nodes to have the same trigger group
		mynode.Motion.Adv.TriggerGroup(1);
		myothernode.Motion.Adv.TriggerGroup(1);

		//set the last node to have a different trigger group
		myThirdNode.Motion.Adv.TriggerGroup(2);

		\endcode
	**/
	virtual void TriggerGroup(size_t groupNumber) = 0;

protected:
													/** \cond INTERNAL_DOC **/
	// Construction 
	IMotionAdv(INode &ourNode,
		nodeparam decLimPnum, nodeparam headTailVlimPnum, nodeparam headDxPnum,
		nodeparam tailDxPnum, nodeparam aDxPnum, nodeparam bDxPnum);
	// Destruction
public:
	virtual ~IMotionAdv() {};
													/** \endcond **/
};
//																			  *
//*****************************************************************************



//*****************************************************************************
// NAME																	      *
// 	IMotion Class
/**
	\brief Motion Control Feature Interface.

	This class object allows setup, initiation and cancellation of motion.
**/
class MN_EXPORT IMotion : public IObjWithNode
{
public:
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Kinematic Constraint Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Current Node Acceleration Limit

		Access the acceleration limit value for the next issued move. The value
		is specified using this node's user acceleration units. Changed values will
		apply to the next move.  The default units are RPM/sec
	
		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the Acceleration Limit
		myNode.Motion.AccLimit = 10000.;				// Sets
		double accLim = myNode.Motion.AccLimit;	// Gets
		\endcode

		\see INode::AccUnit(enum _accUnits newUnits) Changing Acceleration Units
	**/
	ValueDouble AccLimit;

	/**
		\brief Current Node Velocity Limit

		Access the current velocity limit value specified in the current velocity
		units. Changed values will apply to the next issued move.  The default units 
		are RPM.

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the Velocity Limit
		myNode.Motion.VelLimit = 1000.;				// Sets
		double velLim = myNode.Motion.VelLimit;	// Gets
		\endcode

		\see INode::VelUnit(enum _velUnits newUnits)
	**/
	ValueDouble VelLimit;

													/** \cond SC_EXPERT **/
	/**
		\brief Get the current jerk limit register.
	**/
	ValueUnsigned JrkLimit;

	/**
		\brief Get the current delay added by the jerk limit.
	**/
	ValueDouble JrkLimitDelay;
													/** \endcond  **/
	/**
		\brief Post-motion dwell time in milliseconds.

		Access to the post move dwell time used in positional moves with a after-move dwell.

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the Post-motion dwell time.
		myNode.Motion.DwellMs = 100.;				// Sets
		double myDwell = myNode.Motion.DwellMs;	// Gets
		\endcode

		\see MovePosnStart() for more information on how to command a positional move with dwell

	**/
	ValueUnsigned DwellMs;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Motion Duration Calculation Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Calculate the duration of a positional move.

		\param[in] target Target position. This is either the
		absolute position or relative to the current position
		depending on the \a targetIsAbsolute argument.
		\param[in] targetIsAbsolute The target is an absolute
		destination.

		Using the currently specified kinematic constraints, return the
		expected duration (in milliseconds) of the move from when it
		starts to when the move command completes.

		This function is useful for setting up timeouts or helping to schedule
		other events in your application.

		\CODE_SAMPLE_HDR
		// SAMPLE: Using MovePosnDurationMsec to set up a reasonable timeout for a move to complete

		//define a timeout in case the node is unable to complete the move
		double timeout =  mynode.Motion.MoveDurationMsec(1000) + 100;  
		double StartTime = myMgr.TimeStampMsec();

		//This will loop checking to see if the node went Move done with the defined timeout
		while (!mynode.Motion.MoveWentDone() && myMgr.TimeStampMsec() < StartTime+timeout) 
		{//Wait for move to complete
		}

		\endcode

	**/
	virtual double MovePosnDurationMsec(int32_t target,
		bool targetIsAbsolute = false);

	/**
		\brief Calculate the duration of a velocity move.

		\param [in] target Target velocity.

		Using the currently specified kinematic constraints, return the
		expected duration (in milliseconds) of the move's acceleration to the target velocity.

		This function is useful for setting up timeouts or helping to schedule
		other events in your application.

		\CODE_SAMPLE_HDR
		// SAMPLE: Using MoveVelDurationMsec to set up a reasonable timeout for a velocity move to complete

		//define a timeout in case the node is unable to complete the move
		double timeout = mynode.Motion.MoveVelDurationMsec() + 100;
		double StartTime = myMgr.TimeStampMsec();

		//This will loop checking to see if the node went Move done with the defined timeout
		while (!mynode.Motion.VelocityReachedTarget() && myMgr.TimeStampMsec() < StartTime+timeout)
		{//Wait for move to complete
		}

		\endcode
	**/
	virtual double MoveVelDurationMsec(double target);


	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Motion Initiating Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Initiate a positional move.

		\param [in] target Target position. This is either the
		absolute position of relative to the current position
		depending on the \a targetIsAbsolute argument.
		\param [in] targetIsAbsolute the target is an absolute
		destination.
		\param [in] addPostMoveDwell delay the next move being issued
		by DwellMs after this move profile completes.
		\return Number of additional moves the node will accept.

		The move is kinematically limited by the current
		[acceleration](@ref AccLimit), and [velocity](@ref VelLimit) limits.

		Move requests are buffered and requests initiated during an active move will execute
		immediately after the active move completes. The return value
		will tell you the number of additional moves that may be queued(up to 16) when
		this request is processed.  You can also use the <i>Status
		Register's</i> #_cpmStatusRegFlds::MoveBufAvail field, which will be
		deasserted when the buffer is full.
	**/
	virtual size_t MovePosnStart(int32_t target,
		bool targetIsAbsolute = false,
		bool addPostMoveDwell = false) = 0;

	/**
		\brief Initiate a velocity move.

		\param [in] target Target velocity.
		\return Number of additional moves the node will accept.

		The move is kinematically limited by the current
		[acceleration limit](@ref sFnd::IMotion::AccLimit).

		Move requests are buffered and requests initiated during a active move will execute
		immediately after the active move completes. The return value
		will tell you the number of additional moves that may be queued(up to 16) when
		this request is processed. You can also use the <i>Status
		Register's</i> #_cpmStatusRegFlds::MoveBufAvail field, which will be
		deasserted when the buffer is full.
	**/
	virtual size_t MoveVelStart(double target) = 0;

	/**
		\brief Update the status rise register and test and clear the move
		done field.

		Update the [status rise register](@ref sFnd::IStatus::Rise)  and test and clear the node
		[Move Done](@ref cpmStatusRegFlds::MoveDone) field.

		\return true if [Move Done](@ref cpmStatusRegFlds::MoveDone) went true since last call.


		\CODE_SAMPLE_HDR
		// SAMPLE: This checks if the Node has reached move done since the last time the application checked.
		if(mynode.Motion.VelocityReachedTarget())
		{
		printf("the Node reached move done");
		}
		else
		{
		printf("the Node has not reached move done");
		}

		\endcode

		\note For performance reasons it may be superior to use the [Status reference](@ref sFnd::IStatus::Rise) 
		and perform a Refresh in a periodic place in your
		application and check the results via the [Status object](@ref sFnd::ValueStatus).
	**/
	virtual bool MoveWentDone() = 0;

	/**
		\brief Update the real-time status register and test for move done.

		Refresh the [real-time status register](@ref sFnd::IStatus::RT) and test for [Move Done](@ref cpmStatusRegFlds::MoveDone). 
		This allows for a simple script-like interface.


		\return true if [Move Done](@ref cpmStatusRegFlds::MoveDone) is true.

		\CODE_SAMPLE_HDR
		// SAMPLE: This checks if the Node is move done.
		if(mynode.Motion.MoveIsDone())
		{
		printf("the Node is move done");
		}
		else
		{
		printf("the Node is not move done");
		}

		\endcode

		\note For performance reasons it may be superior to use the  [real-time status register](@ref sFnd::IStatus::RT) reference
		and perform a Refresh in a periodic place in your application and check the results via the [Status object](@ref sFnd::ValueStatus).
	**/
	virtual bool MoveIsDone() = 0;

	/**
		\brief Update the status rise register and test and clear the 
		At Target Velocity field.

		Update the [status rise register](@ref sFnd::IStatus::Rise)  and test and clear the node
		[At Target Velocity](@ref cpmStatusRegFlds::AtTargetVel) field.

		\return true if <i>At Target Velocity</i> went true since last call.


		\CODE_SAMPLE_HDR
		// SAMPLE: This checks if the Node has reached the target velocity since the last time the application checked.
		if(mynode.Motion.VelocityReachedTarget())
		{
		printf("the Node reached target velocity");
		}
		else
		{
		printf("the Node has not reached the target velocity");
		}

		\endcode

		\note For performance reasons it may be superior to use the [status rise register](@ref sFnd::IStatus::Rise) reference
		and perform a Refresh in a periodic place in your
		application and check the results via the [Status object](@ref sFnd::ValueStatus).


	**/
	virtual bool VelocityReachedTarget() = 0;

	/**
		\brief Update the real-time status register and test for At 
		Target Velocity.

		Refresh the [real-time status register](@ref sFnd::IStatus::RT) and test for [At Target Velocity](@ref cpmStatusRegFlds::AtTargetVel). 
		This allows for a simple script-like interface.

		\return true if [At Target Velocity](@ref cpmStatusRegFlds::AtTargetVel) is true.

		\CODE_SAMPLE_HDR
		// SAMPLE: This checks if the Node is at the commanded velocity
		if(mynode.Motion.VelocityAtTarget())
		{
		printf("the Node is at velocity");
		}
		else
		{
		printf("the Node is not at velocity");
		}

		\endcode

		\note For performance reasons it may be necessary to use the [Status reference](@ref sFnd::IStatus::RT) 
		and manually perform a Refresh in a periodic place if your application and check the results.
	**/
	virtual bool VelocityAtTarget() = 0;

	/**
		\brief Update the status rise register and test and clear the node
		not ready field.

		Update the [status rise register](@ref sFnd::IStatus::Rise)  and test and clear the node
		 [Not Ready](@ref cpmStatusRegFlds::NotReady) field.

		 \CODE_SAMPLE_HDR
		 // SAMPLE: This checks if the Node has gone not ready since the last time the application checked.
		 if(mynode.Motion.WentNotReady())
		 {
		 printf("the Node went not ready");
		 }
		 else
		 {
		 printf("the Node has been and still is ready");
		 }

		 \endcode

		 \note For performance reasons it may be superior to use the [status rise register](@ref sFnd::IStatus::Rise) 
		 reference and perform a Refresh in a periodic place in your
		 application and check the results via the [Status object](@ref sFnd::ValueStatus).
	**/
	virtual bool WentNotReady() = 0;

	/**
		\brief Update the real-time status register and test for Node ready.

		Refresh the [real-time status register](@ref sFnd::IStatus::RT) and test for [Node Ready](@ref cpmStatusRegFlds::NotReady). 
		This allows for a simple script-like interface.

		\return true if the Node is [ready](@ref cpmStatusRegFlds::NotReady) for motion command.

		\CODE_SAMPLE_HDR
		// SAMPLE: This checks if the Node is ready for motion commands
		if(mynode.Motion.IsReady())
		{
			printf("the Node is ready");
		}
		else
		{
			printf("the Node is not ready");
		}

		\endcode

		\note For performance reasons it may be necessary to use the [Status reference](@ref sFnd::IStatus::RT)
		and manually perform a Refresh in a periodic place if your application and check the results via the Status object.
	**/
	virtual bool IsReady() = 0;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Motion Cancel Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	/**
		\brief Initiate a simple node stop at this node.

		\param[in] howToStop One of the commonly used <i>Node Stop codes.</i>

		This is the simplest way to command a nodestop.

		\CODE_SAMPLE_HDR
		// SAMPLE: This issues an abrupt nodestop on this node
		mynode.Motion.NodeStop(STOP_TYPE_ABRUPT);		

		// SAMPLE: This issues a nodestop to ramp to zero velocity using the current decel parameter's on this node
		mynode.Motion.NodeStop(STOP_TYPE_RAMP_WITH_DECEL);		

		\endcode

		\see [Node Stop Page](@ref CPMnodeStopPage) for a complete list of stop types
		\see sFnd::IMotion::NodeStopClear To clear latching attributes

	**/
	virtual void NodeStop(nodeStopCodes howToStop) = 0;
														/** \cond ISC_DOC **/
	/**
		\brief Initiate a node stop at this node

		\param[in] howToStop Reference to a <i>Node Stop Register</i> object 
		specifying how to stop this node.

		This is the most flexible form of <i>Node Stop</i>. It is used to initiate
		or clear latching modifiers, such as the E-Stop condition.

		\see sFnd::IMotion::NodeStopClear To clear latching attributes

	**/
	virtual void NodeStop(mgNodeStopReg &howToStop) = 0;
													/** \endcond **/

	/**
		\brief Initiate a node stop on all nodes on this port.

		\param[in] howToStop One of the commonly used <i>Node Stop codes.</i>

		This is the simplest way to broadcast a group node stop. This issues
		a stop of the specified type to all nodes on the initiating node's port.

		\see sFnd::IMotion::NodeStopClear To clear latching attributes

	**/
	virtual void GroupNodeStop(nodeStopCodes howToStop) = 0;

	/**
		\brief Clear all latching Node Stop modifiers.

		This function will clear the MotionLock, E-Stop, Controlled, Disabled, 
		etc. latching conditions from the node. This will allow
		normal operations to continue, unless the motor is shutdown state.

		\CODE_SAMPLE_HDR
		// SAMPLE: Clear any Current NodeStops
		myNode.Motion.NodeStopClear();
		\endcode

		\see sFnd::IMotion::NodeStop(nodeStopCodes) For initiating Node Stops
	**/
	virtual void NodeStopClear() = 0;
														/** \cond SC_EXPERT **/
	/**
		\brief Access the stopping acceleration limits in step/sec^2.
	**/
	ValueDouble NodeStopDecelLim;
														/** \endcond  **/
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Position Status Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Access the current measured position.

		Access the current measured position.

		\CODE_SAMPLE_HDR
		// SAMPLE: Reading the current measured position.
		double myPosn = myNode.Motion.PosnMeasured;
		\endcode

		\note The resolution of this value will be the same as the positioning resolution of the motor (800cnts/rev for "Regular" 6400cnts/rev for "Enhanced").
		This means that for step+direction modes the position may be scaled differently from the step and direction input  based on the ratio of the input 
		resolution set up through ClearView, and the motor's native positioning resolution.  For example, if you are using a "Regular" ClearPath-SC motor
		with a native positioning resolution of 800 cnts/rev, and you set the Input resolution to 200 steps/rev to drop in for a 200 cnt stepper motor, each
		step sent will change the position 4 cnts.
	**/
	ValueDouble PosnMeasured;

	/**
		\brief Access the current commanded position.

		Access the current commanded position.

		\CODE_SAMPLE_HDR
		// SAMPLE: Reading the current commanded position.
		double myPosnCmd = myNode.Motion.PosnCommanded;
		\endcode

		\note The resolution of this value will be the same as the positioning resolution of the motor (800cnts/rev for "R" 6400cnts/rev for "E").
		This means that for step and direction modes, the position may be scaled differently from the step and direction input  based on the ratio of the input 
		resolution set up through ClearView, and the motor's native positioning resolution.  For example, if you are using a "R" ClearPath-SC motor
		with a native positioning resolution of 800 cnts/rev, and you set the Input resolution to 200 steps/rev to drop in for a 200 cnt stepper motor, each
		step sent will change the position 4 cnts.
	**/
	ValueDouble PosnCommanded;

	/**
		\brief Access the current position tracking error.

		Access the current position tracking error.  Tracking error is defined as commanded position - measured position.

		\CODE_SAMPLE_HDR
		// SAMPLE: Reading the current position tracking error.
		double myPosnErr = myNode.Motion.PosnTracking;
		\endcode

		\note The resolution of this value will be the same as the positioning resolution of the motor (800cnts/rev for "Regular" 6400cnts/rev for "Enhanced").
		This means that for step+direction modes the position may be scaled differently from the step and direction input  based on the ratio of the input 
		resolution set up through ClearView, and the motor's native positioning resolution.  For example, if you are using a "Regular" ClearPath-SC motor
		with a native positioning resolution of 800 cnts/rev, and you set the Input resolution to 200 steps/rev to drop in for a 200 cnt stepper motor, each
		step sent will change the position 4 cnts.
	**/
	ValueDouble PosnTracking;

	/**
		\brief Adjust number space.

		\param[in] adjAmount The amount to adjust the number space.

		This function is used mostly with a manual homing process, or anytime an application need to adjust the number space. It allows
		the [measured](@ref PosnMeasured)  and [commanded](@ref PosnCommanded) position to be shifted by the specified amount.

		\CODE_SAMPLE_HDR
		// SAMPLE: Zeroing the number space around the current Measured Position
		myNode.Motion.AddToPosition(-myNode.Motion.PosnMeasured);
		\endcode

		\note If using Soft Position limits, this command will
		reset the motor's ["Was Homed"](@ref  _cpmStatusRegFlds::WasHomed) status causing the soft
		limit settings to be ignored.  To re-enable soft limits see the \ref IHoming::SignalComplete function.

	**/
	virtual void AddToPosition(double adjAmount) = 0;

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Velocity Status Group
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Access the current measured velocity.

		Access the current measured velocity. The velocity is scaled to the
		current node's velocity unit.  The default units are RPM.

		\CODE_SAMPLE_HDR
		// SAMPLE: Reading the current measured velocity.
		double myVel = myNode.Motion.VelMeasured;
		\endcode

		\see INode::VelUnit(_velUnits) To set velocity units.
	**/
	ValueDouble VelMeasured;

	/**
		\brief Access the current commanded velocity.

		Access the current commanded velocity. The velocity is scaled to the
		current node's velocity unit. The default units are RPM.

		\CODE_SAMPLE_HDR
		// SAMPLE: Reading the current commanded velocity.
		double myVelCmd = myNode.Motion.VelCommanded;
		\endcode

		\see INode::VelUnit(_velUnits) To set velocity units.
	**/
	ValueDouble VelCommanded;

	/**
		\brief Access the current measured torque.

		Access the current measured torque. The torque is scaled to the 
		current node's torque unit. The default units are percentage of maximum.

		\CODE_SAMPLE_HDR
		// SAMPLE: Reading the current measured torque.
		double myTrq = myNode.Motion.TrqMeasured;
		\endcode

		\see INode::TrqUnit(_trqUnits)
	**/
	ValueDouble TrqMeasured;

	/**
		\brief Access the current commanded torque.

		Access the current commanded torque. The torque is scaled to the 
		current node's torque unit. The default units are percentage of maximum.

		\CODE_SAMPLE_HDR
		// SAMPLE: Reading the current commanded Torque.
		double myTrqCmd = myNode.Motion.TrqCommanded;
		\endcode

		\see INode::TrqUnit(_trqUnits)
	**/
	ValueDouble TrqCommanded;

	/**
		\brief Access advanced features if available on the node's model.
	**/
	IMotionAdv &Adv;

	/**
		\brief Access to the homing feature.
	**/
	IHoming &Homing;
protected:
													/** \cond INTERNAL_DOC **/
	// Construction 
	IMotion(INode &ourNode, IMotionAdv &adv, IHoming &homing,
		nodeparam accLimPnum, nodeparam velLimPnum, nodeparam stopDecLimPnum,
		nodeparam posnMeasPnum, nodeparam posnCmdPnum, nodeparam posnTrkPnum,
		nodeparam velMeasPnum, nodeparam velCmdPnum,
		nodeparam trqMeasPnum, nodeparam trqCmdPnum, nodeparam jrkLimPnum,
		nodeparam dwellPnum, nodeparam jrkDelayPnum);
	// Destruction
public:
	virtual ~IMotion() {};
													/** \endcond **/
};
//																			  *
//*****************************************************************************

//*****************************************************************************
// NAME																	      *
// 	IOuts class
/** \cond SC_EXPERT **/
/**

	\brief Output Register Interface.

	This class manages the access to the output registers.

	This includes:
	- The <i>User Output Register</i>
	- Access to the actual output register state

	Functions are provided to allow thread safe modifications to the
	<i>User Output Register</i>.
**/
class MN_EXPORT IOuts : public IObjWithNode
{
protected:
public:
	// Easy access to enable request
	
	/**
	\copydoc INode::EnableReq(bool)
	**/
	virtual void EnableReq(bool newState) = 0;
	/**
	\copydoc INode::EnableReq(bool)
	**/
	virtual bool EnableReq() = 0;
	// Access to whole output register
	/**
		\brief Access the <i>User Output Register</i>.

		Access the <i>User Output Register</i> object. This is used to 
		change application initiated actions. 

		\note Use the Set and Clear functions to adjust this register when
		multiple threads may be changing this register.
	**/
	ValueOutReg User;
	/**
		\brief Access the <i>Operational Output Register</i>.

		This read-only reference will allow access to the current 
		<i>Operational Output Register</i>. This register is the internal
		register that engages the features attached to it.
	**/
	ValueOutReg Out;
protected:
													
	// Construction
	IOuts(INode &ourNode, nodeparam usrOutPnum, nodeparam outPnum);
	virtual ~IOuts() {};
													
};
			/** \endcond **/
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ISetupEx class
/**
	\brief Expert Setup Interface.

	This class allows access to the more advanced setup options for this node. Items accessed in this class may be overridden by ClearView or 
	configuration file loads.
**/
class MN_EXPORT ISetupEx : public IObjWithNode
{
													/** \cond INTERNAL_DOC **/
	friend class ISetup;
													/** \endcond **/
public:
	/**
		\brief Network watchdog timer. 
		
		This allows the adjustment of a network watchdog timer that will
		force a ramped Node Stop if the host stops communicating with
		the node. This is a safety feature and should not be changed except in 
		rare application specific circumstances.

		The ramp stop acceleration is setup via the E-Stop deceleration rate in ClearView.
		This is found in the details dropdown menu in ClearView.
		
		Set this to zero to disable. Units are milliseconds.

		\note This setting can be [saved into a config file](@ref sFnd::ISetup::ConfigSave), and is overwritten when a [config file is loaded](@ref sFnd::ISetup::ConfigLoad).
	**/
	ValueDouble NetWatchdogMsec;
													/** \cond SC_EXPERT **/
	/**
		\brief Access application setup register.

		This register will not be changed for nearly all applications.
		This register is configured within the motor configuration file, and will not typically change during operation.
	**/
	ValueAppConfigReg App;

	/**
		\brief Access the hardware setup register.

		This register will not be changed for nearly all applications.
		This register is configured within the motor configuration file, and will not typically change during operation.
	**/
	ValueHwConfigReg HW;
														/** \endcond  **/
protected:
													/** \cond INTERNAL_DOC **/
	// Construction
	ISetupEx(INode &ourNode,
		nodeparam appPnum, nodeparam hwPnum, nodeparam netWdPnum);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	ISetup class
/**
	\brief Node Setup Features Interface.
	
This class accesses and manages node setup features. These include:
	- Loading Configuration files from your application
	- Saving Configuration files
**/
class MN_EXPORT ISetup : public IObjWithNode
{
public:
	/**
		\brief Manage the delay to actual disable. 
	
		This allows an electrical brake to engage
		prior to the actual disable. The units are milliseconds.

		\see sFnd::IBrakeControl for more information on configuring, and controlling brakes

		\CODE_SAMPLE_HDR
		// SAMPLE: Getting and setting the delay to disable time.
		myNode.Setup.DelayToDisableMsecs = 100.;				// Sets
		double myDwell = myNode.Setup.DelayToDisableMsecs;	// Gets
		\endcode
	**/
	ValueDouble DelayToDisableMsecs;

	// Configuration Load/Save

	/**
		\brief Load configuration File.

		\param[in] filePath Pointer to C-String file path to the file
		to load.
		\param[in] doReset Set this to true to force the node to reset
		at the end of load.
		
		Load the configuration file specified by \a filePath to
		this node.

		\CODE_SAMPLE_HDR
		// This function will load the Config file. You will have to edit the file path for your application
		char* myconfigFile = "...\MyConfigFolder\MyConfigFile.mtr";
		mynode.Setup.ConfigLoad(myconfigFile);
		\endcode
	**/
	virtual void ConfigLoad(const char *filePath, bool doReset = false) = 0;

	/**
		\brief Save the node's configuration.
		
		\param[in] filePath Pointer to C-String file path to save the
		configuration at.

		Save the node's configuration to the \a filePath specified.  This would typically be performed through ClearView right after tuning each axis. 

		\CODE_SAMPLE_HDR
		//Save the config file before starting to the windows Temp file path
		char tempPath[256];
		GetTempPathA(256, tempPath);
		char myconfigFile[269];
		sprintf(myconfigFile, "%s\myConfig.mtr", tempPath);
		mynode.Setup.ConfigSave(myconfigFile);
		\endcode
	**/
	virtual void ConfigSave(const char *filePath) = 0;

	/**
		\brief Returns true if the node is in full access mode on this network.

		Returns true if the node is in full access mode on this network.  
		The only time a node may not be in full access mode is if ClearView is connected through the USB hub on the motor
		and the user has taken Full Access through the USB diagnostic port via ClearView.

		\CODE_SAMPLE_HDR
		// This example will check to see if the application has full access to the Node
		if(myNode.Setup.AccessLevelIsFull())
		{ ...we have full access. Normal operation can continue...}
		else
		{ ...we don't have full access. Set ClearView to Monitor Mode before continuing...}
		\endcode
	**/
	virtual bool AccessLevelIsFull() = 0;
	/**
		\brief Expert setup features access.
	**/
	ISetupEx Ex;

protected:
													/** \cond INTERNAL_DOC **/
	// Construction
	ISetup(INode &ourNode,
		nodeparam appPnum, nodeparam hwPnum, nodeparam wdogPnum,
		nodeparam delayPnum);
public:
	virtual ~ISetup() {}
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IStatusAdv class
/**
	\brief Advanced Status Features Interface.

	This class accesses and manages the advanced node status information.
	It provides	access to:
	- Capture Registers

	\see \ref PosnCaptPage for information on configuring inputs to capture position
**/
class MN_EXPORT IStatusAdv : public IObjWithNode
{
public:
	/**
	\brief Access the last captured high resolution position value.

	Access the last captured high resolution position value.

	\note This input corresponds to a change of state of the Node's input A
	**/
	ValueSigned CapturedHiResPosn;
	/**
	\brief Return the last at sample-rate position captured.

	Return the last at sample-rate position captured.

	\note This input corresponds to a change of state of the Node's input A
	**/
	ValueSigned CapturedPos;
protected:
													/** \cond INTERNAL_DOC **/
	// Construction
	IStatusAdv(INode &ourNode, nodeparam hiResCapPnum, nodeparam capPnum);
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	IStatus class
/**
	\brief Node Status Features Interface.

	This Node Level class accesses and manages the node status information.  Node Status information is divided into 
	two types of registers:
	- Status Registers
	- Alert/Shutdown Registers

	Status Registers contain current information about the node's dynamic state such as whether a node: is
	in motion, has gone move done, etc. The main Realtime Status Register is maintained by the node
	and is updated each sample-time to reflect the node's state.  To reduce latency and allow the capture of 
	short transient events, the Realtime Status Register feeds a series of accumulating registers. 
	These registers store transitional events until they are accessed by the application program. These are:

	- The deassertions are captured by the Status Fallen Register. 
	- The assertions are captured by the Status Risen & Attention Source Register. This register is the source of the autonomous Attention Packets when enabled on "Advanced" motors. 
	- The Status Accumulated Register is a bitwise OR accumulation of the Realtime Status Register. This register is useful for determining if an assertion has occurred since the last access. 

	
	The Alert Register contains information regarding a shutdown the Node may have experienced during normal operation.
	Depending on the type of alert, the application may be able to recover and/or indentify the root cause of the failure.

	Access to these registers is provided through the [ValueAlert](@ref sFnd::ValueAlert) and [ValueStatus](@ref sFnd::ValueStatus) classes.

	\CODE_SAMPLE_HDR

	//This example code will check a node's status for any current alerts and
	//print out the state string of any alerts found

	//This creates an Empty String for us to put our Alert State String into
	char alertList[256];

	// Refresh the Real-Time Status, and Alert Register
	theNode.Status.RT.Refresh();
	myNode.Status.Alerts.Refresh();


	// Check the Real-Time Status Register's "AlertPresent" bit
	// The bit is set true if there are alerts in the alert register
	if (theNode.Status.RT.Value().cpm.AlertPresent)
	{
	printf("Node has alerts!\n");
	}
	else
	{
	printf("Node has no alerts!\n");
	}

	// get an alertReg reference, and check directly for alerts
	if (myNode.Status.Alerts.Value().isInAlert())
	{
	// Put a text description of all alert bits set into our empty string, then print the string
	myNode.Status.Alerts.Value().StateStr(alertList, 256);
	printf("Node has alerts! Alerts: %s", alertList);

	// Check for specific alerts by accessing the cpmAlertFlds within the alertReg Struct
	if (myNode.Status.Alerts.Value().cpm.TrackingShutdown)
	{
	printf("Tracking Shutdown\n");
	}
	}

	\endcode

	\see sFnd::ValueAlert for more information on accessing the Node's Alert Register
	\see sFnd::ValueStatus for more information on accessing the Node's Status Register
	\see cpmStatusRegFlds for a complete list of ClearPath-SC Status Fields
	\see cpmAlertFlds for a complete list of ClearPath-SC Alerts
**/
class MN_EXPORT IStatus : public IObjWithNode
{
public:
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	// Status Register Instances
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - - 
	/**
		\brief Real Time Status Register Reference.

		The <i>Real Time Status Register</i> reference 
	**/
	ValueStatus RT;

	/**
		\brief Status Risen Register Reference.

		The <i>Status Risen Register</i> reference.
	**/
	ValueStatus Rise;

	/**
		\brief Status Fallen Register Reference.

		The <i>Status Fallen Register</i> reference.
	**/
	ValueStatus Fall;

	/**
		\brief Status Accumulated Register Reference.

		 The <i>Status Accumulated Register</i> reference.
	**/
	ValueStatus Accum;
											/** \cond INTERNAL_DOC **/
	/**
		\brief Warnings Accumulated Register Reference.

		Update and manage <i>Warnings Accumulated Register</i>.
	**/
	ValueAlert Warnings;
											/** \endcond  **/

	/**
		\brief Alerts/Shutdowns Registers Reference.

		The <i>Alerts/Shutdowns Register</i> reference.
	**/
	ValueAlert Alerts;

	/**
		\brief RMS level Reference.

		A reference to the Node's <i>RMS Level</i>.
	**/
	ValueDouble RMSlevel;
	
	/**
	\brief RMS level Reference.

	A reference to the Node's bus and backup power status.
	**/
	ValuePowerReg Power;
	
	/**
		\brief Advanced status information.

		Access to extra status information only available in advanced ClearPath-SC nodes.
	**/
	IStatusAdv &Adv;
public:
	/**
		\brief Clear non-serious alerts.

		Clear all non-serious alerts and allow node to enable if an
		<i>Enable Request</i> is still pending.
	**/
	virtual void AlertsClear() = 0;

	/**
		\brief If true, the node is ready for motion.
		\return True if the node is ready for motion.

		This function is used to determine the availability of the move
		generator to begin operations.
	**/
	virtual bool IsReady() = 0;

	/**
	\brief If true, the node is ready for motion.
	\return True if the node is ready for motion.

	This function is used to determine if a Torque saturation event has
	occurred since the last query. 
	
	\note The <i>Warnings</i> register is queried when this function is called
	**/
	virtual bool HadTorqueSaturation() = 0;


protected:
													/** \cond INTERNAL_DOC **/
	// Construction
	IStatus(INode &ourNode, IStatusAdv &adv,
		nodeparam rtPnum, nodeparam risePnum, nodeparam fallPnum, nodeparam accumPnum,
		nodeparam warnPnum, nodeparam alertPnum, nodeparam rmsLvlPnum,
		nodeparam powerPnum);
public:
	virtual ~IStatus() {}
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	INode class
/**
	\brief Virtual Node Interface.

	This class defines a "super" node whose framework exposes the Node Level features
	available for your application by a series of public attribute references.
	There are common member functions for these feature objects to determine
	if the node supports the feature.

	This class also implements:
	- Velocity Units
	- Acceleration Units
	- Torque Units
	- Enable Requests


	\CODE_SAMPLE_HDR
		// Create instance of sFoundation
		SysManager myMgr;
		// Define a port SC4-HUB attached to
		myMgr.ComHubPort(0, 13);
		// Tell system to open this one port
		myMgr.PortsOpen(1);
		// Create useful reference to the first port object
		IPort &myPort = myMgr.Ports(0);
		// Test for presence of nodes
		if (myPort.NodeCount() == 0) {
			// Bail out, no nodes
			return;
		}
		// If there are nodes present, get a reference to the first one
		INode &myNode =  myPort.Nodes(0);
		// You can now interact with myNode
		myNode.EnableReq(true);
	\endcode

	\see sFnd::SysManager For the root object for interacting with
	ClearPath-SC motors.
	\see sFnd::SysManager::Ports To get references to the IPort objects created
	when SysManager::PortsOpen has successfully opened.
	\see sFnd::IPort::Nodes To get references to INode objects detected on a
	port.
**/
class MN_EXPORT INode
{
													/** \cond INTERNAL_DOC **/
	friend class IInfoEx;
													/** \endcond **/
public:
	/**
		\brief A Simple and Safe Mutex mechanism
		
		This mechanism is used to create critical sections of code when multiple 
		threads are accessing a node, in order to perform read-modify-write 
		operations by one of the threads, for example. An instance of this 
		simplifies error handling whether node mutex is utilized.

		\note This instance should generally be created in a C++ code block
		that requires the node mutex. It must go out of scope before the
		code stream after the mutually exclusive tasks are complete. This
		ensures the release of the mutex in the case of operational failures
		that causes the called function to throw an error.
	
		\CODE_SAMPLE_HDR
		// This function will lock out any other parallel attempts to 
		// access the node until it returns.
		void myFuncWithLock(INode &myNode) {
			// Lock is taken automatically
			INode::UseMutex myLock(myNode);			

			// Perform read-modify-write action

			// Exiting this function guarantees unlock occurs
		}
		\endcode

		\note Care should be taken to hold this instance as short as possible
		to avoid starving competing threads from executing.
	**/
	class MN_EXPORT UseMutex {
	private:
		/// Hold a reference to our node class's internal mutex.
		INode &m_node;
	public:
		/**
			\brief Create a safe and simple mutex

			\param[in,out] ourNode This is passed a reference to the node
			you wish to perform the mutual exclusion on.
		**/
		UseMutex(INode &ourNode);
													/** \cond INTERNAL_DOC **/
		~UseMutex();
													/** \endcond **/
	};
													/** \cond INTERNAL_DOC **/
	/**
		Internal event handler
	**/
	class MN_EXPORT MyEvent {
	private:
		// Hold a reference to our node class's internal mutex.
		INode &m_node;
		int32_t m_eventCount;
	public:
		MyEvent(INode &ourNode);
		bool Wait(size_t timeoutMs);
		void Signal();
		void Register();
		void Unregister();
		int32_t Count();
		void Reset(int32_t count);
	};
													/** \endcond **/
protected:
													/** \cond INTERNAL_DOC **/
	// Address of this node
	multiaddr m_addr;
													/** \endcond **/


	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Acceleration unit handling
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
	/**
		\brief Units for Accelerations parameters and status.
	**/
	enum _accUnits {
		RPM_PER_SEC,	///< Revolutions/Minute/Second 
		COUNTS_PER_SEC2	///< Counts/Second^2
	};
protected:
													/** \cond INTERNAL_DOC **/
	/**
	Current unit storage.
	**/
	enum _accUnits m_AccUnit;
													/** \endcond **/
public:
	/**
		\brief Change the acceleration units

		\param[in] newUnits Change the acceleration units to \a newUnits.

		\CODE_SAMPLE_HDR
		// Change the Units of Acceleration to RPM/Sec
		mynode.AccUnit(mynode.RPM_PER_SEC);

		// Change the Units of Acceleration to Cnts/sec/sec
		mynode.AccUnit(mynode.COUNTS_PER_SEC2);
		\endcode

	**/
	virtual void AccUnit(enum _accUnits newUnits) = 0;
	/**
		\brief Get the current velocity units

		\return Current acceleration unit
	**/
	enum _accUnits AccUnit() { return m_AccUnit; }
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Velocity unit handling
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
	/**
		\brief Units for velocity parameters and status.
	**/
	enum _velUnits {
		RPM,			///< Revolutions/Minute 
		COUNTS_PER_SEC	///< Counts/Second
	};
protected:
													/** \cond INTERNAL_DOC **/
	/**
		\brief Current unit storage.
	**/
	enum _velUnits m_VelUnit;
													/** \endcond **/
public:
	/**
		\brief Change the velocity units

		\param[in] newUnits Set new velocity unit

		\CODE_SAMPLE_HDR
		// Change the Units of Velocity to RPM
		mynode.VelUnit(mynode.RPM);

		// Change the Units of Velocity to Cnts/Sec
		mynode.VelUnit(mynode.COUNTS_PER_SEC);
		\endcode


	**/
	virtual void VelUnit(enum _velUnits newUnits) = 0;
	/**
		\brief Get the current velocity units

		\return Current velocity unit
	**/
	enum _velUnits VelUnit() { return m_VelUnit; }
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Torque unit handling
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
	/**
		\brief Units for torque parameters and status.
	**/
	enum _trqUnits {
		PCT_MAX,		///< Percentage of drive maximum
		AMPS			///< Drive delivery amps
	};
protected:
													/** \cond INTERNAL_DOC **/
	/**
		\Current unit storage.
	**/
	enum _trqUnits m_TrqUnit;
													/** \endcond **/
public:
	/**
		\brief Change the torque units

		\param[in] newUnits New torque unit

		\CODE_SAMPLE_HDR
		// Change the Units of torque to percent of max
		mynode.AccUnit(mynode.PCT_MAX);

		// Change the Units of torque to Amps
		mynode.AccUnit(mynode.AMPS);
		\endcode
	**/
	virtual void TrqUnit(enum _trqUnits newUnits) = 0;
	/**
		\brief Get the current torque units

		\return Current torque unit
	**/
	enum _trqUnits TrqUnit() { return m_TrqUnit; }

	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
	// Feature Access
	// - - - - - - - - - - - - - - - - - - - - - - - - - - - - - -
public:
	/**
		\brief Get a reference to the port object this node is attached to.
	**/
	IPort &Port;
	/**
		\brief This reference allows for access about generic information
		about a node.
	**/
	IInfo &Info;
	/**
		\brief Get a reference to the motion feature interface object
	**/
	IMotion &Motion;
	/**
		\brief Get a reference to the status information object
	**/
	IStatus &Status;
	/**
		\brief Get a reference to the limits feature object
	**/
	ILimits &Limits;
															/** \cond INTERNAL_DOC **/
	/**
		\brief Get a reference to the outputs object
	**/
	IOuts &Outs;
															/** \endcond **/
	/**
		\brief Get a reference to our Setup object
	**/
	ISetup &Setup;
															/** \cond SC_EXPERT **/
	/**
		\brief Get a reference to our expert features object
	**/
	INodeEx &Ex;
															/** \endcond **/
	/**
		\brief Get a reference to the advanced features object
	**/
	INodeAdv &Adv;

	// Easy access to enable request
	/**
	\brief Thread safe enable request
	\param[in] newState Change the state of the <i>Enable Request</i>.

	This function allows you to assert the user <i>Enable Request</i>.  If <i>Enable Request</i>
	is asserted the drive will enable unless an overriding disable nodeStop condition or a shutdown persists.
	If a [shutdown, or nodestop](@ref CPMnodeStopPage) prevents the Node from enabling, then as soon as the condition is removed by 
	[clearing the Nodestop](@ref sFnd::IMotion::NodeStopClear) or [clearing the alert](@ref sFnd::IStatus::AlertsClear)
	The Node will immediately enable(there is no need to re-send the enable request).

	**/
	void EnableReq(bool newState) {
		Outs.EnableReq(newState);
	}
	/**
	\brief Get the current <i>Enable Request</i> state.

	This function will return the state of the user <i>Enable Request</i>.

	\note The Node may not be Enabled even if this request is asserted.  A [NodeStop](@ref CPMnodeStopPage), or [Shutdown Alert](@ref sFnd::ValueAlert)
	can prevent the enable request from executing.

	**/
	bool EnableReq(){
		return Outs.EnableReq();
	}
protected:
													/** \cond INTERNAL_DOC **/
	// Construction
	INode(IPort &port, multiaddr theAddr,
		IInfo &info, IMotion &motion, IStatus &status,
		ILimits &limits, IOuts &outs, INodeAdv &adv, ISetup &setup,
		INodeEx &ex);
public:
	// Destruct
	virtual ~INode() {};
													/** \endcond **/
};
//																			  *
//*****************************************************************************


//*****************************************************************************
// NAME																	      *
// 	SysManager class 
/**
	\brief sFoundation Top Level System Object.

	This class defines the manager object whose framework implements system level functions including:
	- Functions to open, close, and restart ports in the system
	- A thread safe system dwell
	- The system timestamp
	- Direct access to ports and nodes within the system

	Only one instance of this object should be used in any application.

	\CODE_SAMPLE_HDR
	// Create instance of sFoundation
	SysManager myMgr;
	// Define a port SC4-HUB attached to
	myMgr.ComHubPort(0, 13);
	// Tell system to open this one port
	myMgr.PortsOpen(1);
	// Access Node objects through the port
	myMgr.Ports(0).Nodes(0);
	\endcode

	\see \ref sFnd::IPort to reference port level features
	\see \ref sFnd::INode to reference node level features
	\see \ref ClassOverviewPage for a high level overview of how these libraries interact
**/
class MN_EXPORT SysManager
{
public:
	/**
		\brief A  safe thread locking mechanism

		This mechanism is used to allow debugging in a multi-threaded 
		application. It locks a gate to prevent commands from being issued, 
		then waits for all outstanding commands to finish and receive their 
		responses from the Nodes. It then locks the mutex to the current thread, preventing 
		other threads from executing. Doing this prevents response timeouts from 
		occurring while stepping through code.

		\note This instance should generally be created in a C++ code block
		prior to a placed breakpoint within the same scope. When stepping 
		through code, leaving the scope of where the ThreadLock was taken will unlock the other threads; 
		possibly resulting in a timeout.

		\note It may also be advisable when debugging to turn off the [watchdog timers](@ref sFnd::ISetupEx) for each Node.

		\CODE_SAMPLE_HDR
		// This example shows how you can use ThreadLocks in
		// conjuction with breakpoints to debug your application.
		void myFunction() {

		<<<<<<  function code Here <<<<<<<<<<<<<<

		// Insert this just before the Break point to lock other threads to prevent response timeouts
		SysManager::ThreadLock lock;

		<<<<  insert breakpoint Here <<<<<<<<

		<<<<<< more function code Here <<<<<<<<<<<<<<
		}
		//Once the scope of the function is left, the ThreadLock is released allowing other Threads to function normally.
		\endcode
	**/
	class MN_EXPORT ThreadLock {
	public:
		/**
			\brief Create a safe and simple locking mutex to block other
					threads from execution.
		**/
		ThreadLock();	
													/** \cond INTERNAL_DOC **/
		~ThreadLock();
													/** \endcond **/
	};
													/** \cond INTERNAL_DOC **/
private:
	/*
	Port Specifications
	*/
	portSpec m_ports[NET_CONTROLLER_MAX];
	// Count specified from last PortsOpen call
	size_t m_portOpenCount;
													/** \endcond **/
public:

	
	/**
		\brief Search for ClearPath-SC COM Hubs on this machine

		\param[out] comHubPorts This will hold a vector of strings 
		which can later be passed to ComHubPort to open the given port(s).
	**/
	static void FindComHubPorts(std::vector<std::string>& comHubPorts);

	#if defined(_WIN32)||defined(_WIN64)
	/**
		\brief Simplified setup for ClearPath-SC COM Hub

		\param[in] netNumber This is the index of the port to setup. The first
		port is numbered zero. The range of the netNumbers is from 0 to 2.
		\param[in] portNumber This is Windows oriented port number. i.e.
		COM1 would set \a portNumber = 1.
		\param[in] portRate This is set to the desired network rate to operate
		at. If using a SC-HUB, you can specify the speed as MN_BAUD_24X, else
		you should use MN_BAUD_12X for 115200 fields/second ports typically
		found on a PC motherboard.

		This function is used to define the relationship between the system's
		serial port number and the index number(netNumber) in your system.
		\CODE_SAMPLE_HDR
		// This example configures the first port(netNumber=0) to refer to windows COM5, using the default baud rate of 115200
		myMgr.ComHubPort(0,5);

		// This example configures the 2nd port(netNumber=1) to refer to windows COM10, using the higher baud rate of 230400
		myMgr.ComHubPort(1,10,MN_BAUD_24X);
		\endcode
	**/
	void ComHubPort(size_t netNumber,
		int portNumber,
		netRates portRate = MN_BAUD_12X);
	#endif
													/** \cond INTERNAL_DOC **/
	void ComHubPort(size_t netNumber,
		const char *portPath,
		netRates portRate = MN_BAUD_12X);
													/** \endcond **/
													/** \cond INTERNAL_DOC **/
	/**
		\brief Get a reference to port's setup

		\param[in] netNumber The index into the port table. The first port is
		numbered zero. The valid range is [0...NET_CONTROLLER_MAX-1].

		Returns the current settings for the specified port.
	**/
	portSpec &PortSetup(size_t netNumber);
													/** \endcond **/

	/**
		\brief Open ports to start operations.

		\param[in] portCount Number of ports specified

		Call this function to start network operations after the ports have
		been specified.

		\note Ports should be assigned from 0 without skipping
		any.

		\CODE_SAMPLE_HDR
		// Create our system instance
		SysManager myMgr;
		// Set the first port = COM10
		myMgr.ComHubPort(0, 10);
		// Set the second port = COM12
		myMgr.ComHubPort(1, 12);
		// Start system with two ports.
		myMgr.PortsOpen(2);
		\endcode

		\see IPort::OpenState For information about starting success.
		\see IPort::WaitForOnline To synchronize waiting for online state.
	**/
	void PortsOpen(size_t portCount);

	/**
		\brief Open ports to start operations.

		\copydetails PortsOpen(size_t)
	*/
	void PortsOpen(int portCount);

	/**
		\brief Close all operations down and close the ports.

		This function will close the serial ports in the system and release
		all resources related to them.
		
		\note Any references to Ports, Nodes, or ValueXXXX parameters will no longer be valid after this function is called
	**/
	void PortsClose();

	/**
		\brief Close and re-open ports with node restarts and perform system 
		initialization.
		
		This function will send a reset node command to restart them
		to their powered-up state. The serial ports are re-inventoried for 
		nodes and the SysManager refreshed for a new startup.

		\note Any references to Ports, Nodes, or ValueXXXX parameters will no longer be valid after this function is called

		\see IPort::OpenState For information about starting success.
		\see IPort::WaitForOnline To synchronize waiting for online state.
		**/
	void RestartCold();

	/**
		\brief Close and re-open ports and perform system initialization.

		This function will close and re-open the serial ports are 
		re-inventoried for nodes and the SysManager refreshed for a new startup.

		\note Any references to Ports, Nodes, or ValueXXXX parameters will no longer be valid after this function is called

		\see IPort::OpenState For information about starting success.
		\see IPort::WaitForOnline To synchronize waiting for online state.
		**/
	void RestartWarm();

	/**
		\brief Get a time-stamp.

		\return High precision time-stamp

		This time-stamp will have at least millisecond resolution. For the
		Windows platform this is actually microsecond resolution.

		\note This time stamp correlates to the time stamp in the [Command Trace](@ref sFnd::IPort::CommandTraceSave)  file.

		\CODE_SAMPLE_HDR
		//This example show how you can use the TimeStampMsec function to establish a reasonable timout for a polling loop.

		 double timeout;
        timeout = myMgr.TimeStampMsec() + TimeToWait;         //define a timeout in case the node is unable to enable

        //This will loop checking on some status, and will timeout if the status is not found
        while (!mynode.SomeStatus()) {
            mynode.SomeStatus.Refresh();
            if (myMgr.TimeStampMsec() > timeout) {
                printf("Error: Timed out waiting for some status\n");
                return -2;
            }
        }
		\endcode

	**/
	double TimeStampMsec();

	/**
		\brief Format a std:string with a time-stamp string.

		Updates \a toUpdString with a time-stamp string that correlates to
		the [Command Trace](@ref sFnd::IPort::CommandTraceSave) feature.

		\param[in,out] toUpdString Reference to string to update
		\returns \a toUpdString the incoming string reference

		\CODE_SAMPLE_HDR
		// Create our system instance
		SysManager myMgr;
		// String to store results
		string myString;
		// Print time stamp to cout
		cout << myMgr.TimeStampMsecStr(myString) << endl;
		cout << myString << endl;
		printf("%s\n", myString.c_str());
		\endcode
		**/
	std::string &TimeStampMsecStr(std::string &toUpdString);

	/**
		\brief Format a std:string with a time-stamp string.

		Returns a time-stamp string that correlates to
		the [Command Trace](@ref sFnd::IPort::CommandTraceSave) feature.

		\CODE_SAMPLE_HDR
		// Create our system instance
		SysManager myMgr;
		// Print time stamp to cout
		cout << myMgr.TimeStampMsecStr() << endl;
		printf("%s\n", myMgr.TimeStampMsecStr().c_str());
		\endcode

	**/
	std::string TimeStampMsecStr();

	/**
		\brief Suspend thread execution.

		\param [in] msec Delay in milliseconds.

		This calls the underlying operating system's thread suspension 
		function to delay execution for this thread.

		\CODE_SAMPLE_HDR
		// Create our system instance
		SysManager myMgr;
		// Wait 100 milliseconds
		cout << myMgr.TimeStampMsecStr() << endl;
		myMgr.Delay(100);
		cout << myMgr.TimeStampMsecStr() << endl;
		\endcode
	**/
	void Delay(uint32_t msec);

	/**
		\brief Get reference to our port object.

		\param[in] index The port index used when initializing the system.

		This function is used to get a reference to the IPort object associated
		with serial port. This would be the same as argument used with the 
		SysManager::ComHubPort function.

		\see SysManager::ComHubPort For setting up a serial port,
	**/
	IPort &Ports(size_t index);

	/**
		\brief Convert multi-address to an INode reference. 

		\param[in] theMultiAddr This a system's internal node address.

		The returned node reference may exist on any defined ports. 
		
		\remark A \ref _mnErr object is thrown if the requested address does not
		exist in your system or the port currently is offline.

		\CODE_SAMPLE_HDR
		// Get a reference to a node from a specific address
		// 0-15 correspond to Nodes on port 0
		// 16-31 correspond to Nodes on port 1
		// 32-47 correspond to Nodes on port 2
		INode &mynode = myMgr.NodeGet(3);	// Get the 4th node on port 0
		INode &mynode2 = myMgr.NodeGet(18); // Get the 3rd node on port 1
		\endcode
	**/
	INode &NodeGet(multiaddr theMultiAddr);
													/** \cond INTERNAL_DOC **/
	// Constructor
	SysManager();
	~SysManager();
													/** \endcond **/
};
//																			  *
//*****************************************************************************
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif


//*****************************************************************************
}  // namespace sFnd
//*****************************************************************************
#endif
//============================================================================= 
//	END OF FILE pubSysCls.h
//=============================================================================
