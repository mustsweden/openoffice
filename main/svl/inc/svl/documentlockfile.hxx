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



#ifndef _SVT_DOCUMENTLOCKFILE_HXX
#define _SVT_DOCUMENTLOCKFILE_HXX

#include <svl/svldllapi.h>

#include <com/sun/star/io/XStream.hpp>
#include <com/sun/star/io/XInputStream.hpp>
#include <com/sun/star/io/XOutputStream.hpp>
#include <com/sun/star/io/XSeekable.hpp>
#include <com/sun/star/io/XTruncate.hpp>
#include <com/sun/star/lang/XMultiServiceFactory.hpp>

#include <svl/lockfilecommon.hxx>

namespace svt {

class SVL_DLLPUBLIC DocumentLockFile : public LockFileCommon
{
    // the workaround for automated testing!
    static sal_Bool m_bAllowInteraction;

    ::com::sun::star::uno::Reference< ::com::sun::star::io::XInputStream > OpenStream();

    void WriteEntryToStream( ::com::sun::star::uno::Sequence< ::rtl::OUString > aEntry, ::com::sun::star::uno::Reference< ::com::sun::star::io::XOutputStream > xStream );

public:
    DocumentLockFile( const ::rtl::OUString& aOrigURL, const ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >& xFactory = ::com::sun::star::uno::Reference< ::com::sun::star::lang::XMultiServiceFactory >() );
    ~DocumentLockFile();

    sal_Bool CreateOwnLockFile();
    ::com::sun::star::uno::Sequence< ::rtl::OUString > GetLockData();
    sal_Bool OverwriteOwnLockFile();
    void RemoveFile();

    // the methods allow to control whether UI interaction regarding the locked document file is allowed
    // this is a workaround for automated tests
    static void AllowInteraction( sal_Bool bAllow ) { m_bAllowInteraction = bAllow; }
    static sal_Bool IsInteractionAllowed() { return m_bAllowInteraction; }
};

}

#endif

