/////////////////////////////////////////////////////////////////////////////
// Copyright (c) Electronic Arts Inc. All rights reserved.
/////////////////////////////////////////////////////////////////////////////


#ifndef EASTL_INTERNAL_SMART_PTR_H
#define EASTL_INTERNAL_SMART_PTR_H


#include <EABase/eabase.h>
#include <EASTL/type_traits.h>
#include <EASTL/memory.h>
#if defined(EA_PRAGMA_ONCE_SUPPORTED)
	#pragma once
#endif


namespace eastl
{
	/// smart_ptr_deleter
	///
	/// Deprecated in favor of the C++11 name: default_delete
	///
	template <typename T>
	struct smart_ptr_deleter
	{
		typedef T value_type;

		void operator()(const value_type* p) const // We use a const argument type in order to be most flexible with what types we accept. 
			{ delete const_cast<value_type*>(p); }
	};

	template <>
	struct smart_ptr_deleter<void>
	{
		typedef void value_type;

		void operator()(const void* p) const
			{ delete[] (char*)p; } // We don't seem to have much choice but to cast to a scalar type.
	};

	template <>
	struct smart_ptr_deleter<const void>
	{
		typedef void value_type;

		void operator()(const void* p) const
			{ delete[] (char*)p; } // We don't seem to have much choice but to cast to a scalar type.
	};



	/// smart_array_deleter
	///
	/// Deprecated in favor of the C++11 name: default_delete
	///
	template <typename T>
	struct smart_array_deleter
	{
		typedef T value_type;

		void operator()(const value_type* p) const // We use a const argument type in order to be most flexible with what types we accept. 
			{ delete[] const_cast<value_type*>(p); }
	};

	template <>
	struct smart_array_deleter<void>
	{
		typedef void value_type;

		void operator()(const void* p) const
			{ delete[] (char*)p; } // We don't seem to have much choice but to cast to a scalar type.
	};


} // namespace eastl


#endif // Header include guard
