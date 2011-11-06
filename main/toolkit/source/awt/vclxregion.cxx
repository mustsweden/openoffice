/**************************************************************
 * 
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 * 
 *   http://www.apache.org/licenses/LICENSE-2.0
 * 
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 * 
 *************************************************************/



// MARKER(update_precomp.py): autogen include statement, do not remove
#include "precompiled_toolkit.hxx"


#include <toolkit/awt/vclxregion.hxx>
#include <toolkit/helper/macros.hxx>
#include <toolkit/helper/vclunohelper.hxx>
#include <toolkit/helper/convert.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <rtl/memory.h>
#include <rtl/uuid.h>
#include <vcl/svapp.hxx>

//	----------------------------------------------------
//	class VCLXRegion
//	----------------------------------------------------
VCLXRegion::VCLXRegion()
{
}

VCLXRegion::~VCLXRegion()
{
}

// ::com::sun::star::uno::XInterface
::com::sun::star::uno::Any VCLXRegion::queryInterface( const ::com::sun::star::uno::Type & rType ) throw(::com::sun::star::uno::RuntimeException)
{
	::com::sun::star::uno::Any aRet = ::cppu::queryInterface( rType,
										SAL_STATIC_CAST( ::com::sun::star::awt::XRegion*, this ),
										SAL_STATIC_CAST( ::com::sun::star::lang::XUnoTunnel*, this ),
										SAL_STATIC_CAST( ::com::sun::star::lang::XTypeProvider*, this ) );
	return (aRet.hasValue() ? aRet : OWeakObject::queryInterface( rType ));
}

// ::com::sun::star::lang::XUnoTunnel
IMPL_XUNOTUNNEL( VCLXRegion )

// ::com::sun::star::lang::XTypeProvider
IMPL_XTYPEPROVIDER_START( VCLXRegion )
	getCppuType( ( ::com::sun::star::uno::Reference< ::com::sun::star::awt::XRegion>* ) NULL )
IMPL_XTYPEPROVIDER_END



::com::sun::star::awt::Rectangle VCLXRegion::getBounds() throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	return AWTRectangle( maRegion.GetBoundRect() );
}

void VCLXRegion::clear() throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	maRegion.SetEmpty();
}

void VCLXRegion::move( sal_Int32 nHorzMove, sal_Int32 nVertMove ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	maRegion.Move( nHorzMove, nVertMove );
}

void VCLXRegion::unionRectangle( const ::com::sun::star::awt::Rectangle& rRect ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	maRegion.Union( VCLRectangle( rRect ) );
}

void VCLXRegion::intersectRectangle( const ::com::sun::star::awt::Rectangle& rRect ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	maRegion.Intersect( VCLRectangle( rRect ) );
}

void VCLXRegion::excludeRectangle( const ::com::sun::star::awt::Rectangle& rRect ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	maRegion.Exclude( VCLRectangle( rRect ) );
}

void VCLXRegion::xOrRectangle( const ::com::sun::star::awt::Rectangle& rRect ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	maRegion.XOr( VCLRectangle( rRect ) );
}

void VCLXRegion::unionRegion( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XRegion >& rxRegion ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	if ( rxRegion.is() )
		maRegion.Union( VCLUnoHelper::GetRegion( rxRegion ) );
}

void VCLXRegion::intersectRegion( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XRegion >& rxRegion ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	if ( rxRegion.is() )
		maRegion.Intersect( VCLUnoHelper::GetRegion( rxRegion ) );
}

void VCLXRegion::excludeRegion( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XRegion >& rxRegion ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	if ( rxRegion.is() )
		maRegion.Exclude( VCLUnoHelper::GetRegion( rxRegion ) );
}

void VCLXRegion::xOrRegion( const ::com::sun::star::uno::Reference< ::com::sun::star::awt::XRegion >& rxRegion ) throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	if ( rxRegion.is() )
		maRegion.XOr( VCLUnoHelper::GetRegion( rxRegion ) );
}

::com::sun::star::uno::Sequence< ::com::sun::star::awt::Rectangle > VCLXRegion::getRectangles() throw(::com::sun::star::uno::RuntimeException)
{
	::osl::Guard< ::osl::Mutex > aGuard( GetMutex() );

	sal_uLong nRects = maRegion.GetRectCount();
	::com::sun::star::uno::Sequence< ::com::sun::star::awt::Rectangle > aRects( nRects );

	Rectangle aRect;
	sal_uInt32 nR = 0;
	RegionHandle h = maRegion.BeginEnumRects();
	while ( maRegion.GetNextEnumRect( h, aRect ) )
		aRects.getArray()[nR++] = AWTRectangle( aRect );
	maRegion.EndEnumRects( h );

	return aRects;
}



