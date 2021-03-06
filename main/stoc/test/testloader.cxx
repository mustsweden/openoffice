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
#include "precompiled_stoc.hxx"

#include <stdio.h>

#include <sal/main.h>
#ifndef _OSL_MODULE_H_
#include <osl/module.hxx>
#endif
#include <osl/diagnose.h>

#include <com/sun/star/loader/XImplementationLoader.hpp>
#include <com/sun/star/lang/XServiceInfo.hpp>
#include <com/sun/star/lang/XSingleServiceFactory.hpp>
#include <com/sun/star/lang/XSingleComponentFactory.hpp>

#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/factory.hxx>

#if defined ( UNX )
#include <limits.h>
#define _MAX_PATH PATH_MAX
#endif

using namespace com::sun::star::uno;
using namespace com::sun::star::loader;
using namespace com::sun::star::lang;
using namespace osl;
using namespace rtl;
using namespace cppu;

#if OSL_DEBUG_LEVEL > 0
#define TEST_ENSHURE(c, m)   OSL_ENSURE(c, m)
#else
#define TEST_ENSHURE(c, m)   OSL_VERIFY(c)
#endif

class EmptyComponentContext : public WeakImplHelper1< XComponentContext >
{
public:
	virtual Any SAL_CALL getValueByName( const OUString& /*Name*/ )
		throw (RuntimeException)
		{
			return Any();
		}
    virtual Reference< XMultiComponentFactory > SAL_CALL getServiceManager(  )
		throw (RuntimeException)
		{
			return Reference< XMultiComponentFactory > ();
		}

};


SAL_IMPLEMENT_MAIN()
{
	Reference<XInterface> xIFace;

	Module module;

	OUString dllName(
        RTL_CONSTASCII_USTRINGPARAM("bootstrap.uno" SAL_DLLEXTENSION) );
	
	if (module.load(dllName))
	{
		// try to get provider from module
		component_getFactoryFunc pCompFactoryFunc = (component_getFactoryFunc)
			module.getFunctionSymbol( OUString::createFromAscii(COMPONENT_GETFACTORY) );

		if (pCompFactoryFunc)
		{
			XSingleServiceFactory * pRet = (XSingleServiceFactory *)(*pCompFactoryFunc)(
				"com.sun.star.comp.stoc.DLLComponentLoader", 0, 0 );
			if (pRet)
			{
				xIFace = pRet;
				pRet->release();
			}
		}
	}

	TEST_ENSHURE( xIFace.is(), "testloader error1");

	Reference<XSingleComponentFactory> xFactory( Reference<XSingleComponentFactory>::query(xIFace) );

	TEST_ENSHURE( xFactory.is(), "testloader error2");

	Reference<XInterface> xLoader = xFactory->createInstanceWithContext( new EmptyComponentContext );

	TEST_ENSHURE( xLoader.is(), "testloader error3");

	Reference<XServiceInfo> xServInfo( Reference<XServiceInfo>::query(xLoader) );

	TEST_ENSHURE( xServInfo.is(), "testloader error4");

	TEST_ENSHURE( xServInfo->getImplementationName().equalsAsciiL( RTL_CONSTASCII_STRINGPARAM("com.sun.star.comp.stoc.DLLComponentLoader") ), "testloader error5");
	TEST_ENSHURE( xServInfo->supportsService(OUString( RTL_CONSTASCII_USTRINGPARAM("com.sun.star.loader.SharedLibrary")) ), "testloader error6");
	TEST_ENSHURE( xServInfo->getSupportedServiceNames().getLength() == 1, "testloader error7");

	xIFace.clear();
	xFactory.clear();
	xLoader.clear();
	xServInfo.clear();

	printf("Test Dll ComponentLoader, OK!\n");

	return(0);
}


