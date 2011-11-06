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



#ifndef _SFXVISIBILITYITEM_HXX
#define _SFXVISIBILITYITEM_HXX

#include "svl/svldllapi.h"
#include <tools/debug.hxx>
#include <svl/poolitem.hxx>
#include <com/sun/star/frame/status/Visibility.hpp>

//============================================================================
DBG_NAMEEX_VISIBILITY(SfxVisibilityItem, SVL_DLLPUBLIC)

class SVL_DLLPUBLIC SfxVisibilityItem: public SfxPoolItem
{
	::com::sun::star::frame::status::Visibility m_nValue;

public:
	TYPEINFO();

	SfxVisibilityItem(sal_uInt16 which = 0, sal_Bool bVisible = sal_True):
		SfxPoolItem(which)
	{ 
        m_nValue.bVisible = bVisible;
        DBG_CTOR(SfxVisibilityItem, 0);
    }

	SfxVisibilityItem(sal_uInt16 which, SvStream & rStream);

	SfxVisibilityItem(const SfxVisibilityItem & rItem):
		SfxPoolItem(rItem), m_nValue(rItem.m_nValue)
	{ DBG_CTOR(SfxVisibilityItem, 0); }

	virtual ~SfxVisibilityItem() { DBG_DTOR(SfxVisibilityItem, 0); }

	virtual int operator ==(const SfxPoolItem & rItem) const;

    using SfxPoolItem::Compare;
	virtual int Compare(const SfxPoolItem & rWith) const;

	virtual SfxItemPresentation GetPresentation(SfxItemPresentation,
												SfxMapUnit, SfxMapUnit,
												XubString & rText,
                                                const IntlWrapper * = 0)
		const;

	virtual	sal_Bool QueryValue( com::sun::star::uno::Any& rVal,
							 sal_uInt8 nMemberId = 0 ) const;

	virtual	sal_Bool PutValue( const com::sun::star::uno::Any& rVal,
						   sal_uInt8 nMemberId = 0 );

	virtual SfxPoolItem * Create(SvStream & rStream, sal_uInt16) const;

	virtual SvStream & Store(SvStream & rStream, sal_uInt16) const;

	virtual SfxPoolItem * Clone(SfxItemPool * = 0) const;

	virtual sal_uInt16 GetValueCount() const;

	virtual UniString GetValueTextByVal(sal_Bool bTheValue) const;

    sal_Bool GetValue() const { return m_nValue.bVisible; }

	void SetValue(sal_Bool bVisible) { m_nValue.bVisible = bVisible; }
};

#endif // _SFXVISIBILITYITEM_HXX
