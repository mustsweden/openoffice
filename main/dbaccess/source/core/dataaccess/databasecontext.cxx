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
#include "precompiled_dbaccess.hxx"

#include "apitools.hxx"
#include "core_resource.hrc"
#include "core_resource.hxx"
#include "databasecontext.hxx"
#include "databasedocument.hxx"
#include "databaseregistrations.hxx"
#include "datasource.hxx"
#include "dbastrings.hrc"
#include "module_dba.hxx"

/** === being UNO includes === **/
#include <com/sun/star/beans/NamedValue.hpp>
#include <com/sun/star/beans/PropertyAttribute.hpp>
#include <com/sun/star/beans/XPropertySet.hpp>
#include <com/sun/star/document/MacroExecMode.hpp>
#include <com/sun/star/document/XFilter.hpp>
#include <com/sun/star/document/XImporter.hpp>
#include <com/sun/star/frame/XDesktop.hpp>
#include <com/sun/star/frame/XModel.hpp>
#include <com/sun/star/frame/XModel2.hpp>
#include <com/sun/star/frame/XTerminateListener.hpp>
#include <com/sun/star/lang/DisposedException.hpp>
#include <com/sun/star/registry/InvalidRegistryException.hpp>
#include <com/sun/star/sdbc/XDataSource.hpp>
#include <com/sun/star/task/InteractionClassification.hpp>
#include <com/sun/star/ucb/InteractiveIOException.hpp>
#include <com/sun/star/ucb/IOErrorCode.hpp>
#include <com/sun/star/util/XCloseable.hpp>
/** === end UNO includes === **/

#include <basic/basmgr.hxx>
#include <comphelper/enumhelper.hxx>
#include <comphelper/evtlistenerhlp.hxx>
#include <comphelper/namedvaluecollection.hxx>
#include <comphelper/processfactory.hxx>
#include <comphelper/sequence.hxx>
#include <cppuhelper/implbase1.hxx>
#include <cppuhelper/typeprovider.hxx>
#include <cppuhelper/exc_hlp.hxx>
#include <svl/filenotation.hxx>
#include <tools/debug.hxx>
#include <tools/diagnose_ex.h>
#include <tools/fsys.hxx>
#include <tools/urlobj.hxx>
#include <ucbhelper/content.hxx>
#include <unotools/confignode.hxx>
#include <unotools/pathoptions.hxx>
#include <unotools/sharedunocomponent.hxx>
#include <list>
#include <boost/bind.hpp>

using namespace ::com::sun::star::sdbc;
using namespace ::com::sun::star::sdb;
using namespace ::com::sun::star::beans;
using namespace ::com::sun::star::uno;
using namespace ::com::sun::star::document;
using namespace ::com::sun::star::frame;
using namespace ::com::sun::star::lang;
using namespace ::com::sun::star::container;
using namespace ::com::sun::star::util;
using namespace ::com::sun::star::registry;
using namespace ::com::sun::star;
using namespace ::cppu;
using namespace ::osl;
using namespace ::utl;

using ::com::sun::star::task::InteractionClassification_ERROR;
using ::com::sun::star::ucb::IOErrorCode_NO_FILE;
using ::com::sun::star::ucb::InteractiveIOException;
using ::com::sun::star::ucb::IOErrorCode_NOT_EXISTING;
using ::com::sun::star::ucb::IOErrorCode_NOT_EXISTING_PATH;

//==========================================================================

extern "C" void SAL_CALL createRegistryInfo_ODatabaseContext()
{
	static ::dba::OLegacySingletonRegistration< ::dbaccess::ODatabaseContext > aODatabaseContext_AutoRegistration;
}

//........................................................................
namespace dbaccess
{
//........................................................................

    // .............................................................................
        typedef ::cppu::WeakImplHelper1 <   XTerminateListener
                                        >   DatabaseDocumentLoader_Base;
        class DatabaseDocumentLoader : public DatabaseDocumentLoader_Base
        {
        private:
            Reference< XDesktop >               m_xDesktop;
            ::std::list< const ODatabaseModelImpl* >  m_aDatabaseDocuments;

        public:
            DatabaseDocumentLoader( const comphelper::ComponentContext& _aContext);

            inline void append(const ODatabaseModelImpl& _rModelImpl )
            {
                m_aDatabaseDocuments.push_back(&_rModelImpl);
            }
            inline void remove(const ODatabaseModelImpl& _rModelImpl) { m_aDatabaseDocuments.remove(&_rModelImpl); }

        private:
            // XTerminateListener
            virtual void SAL_CALL queryTermination( const lang::EventObject& Event ) throw (TerminationVetoException, RuntimeException);
            virtual void SAL_CALL notifyTermination( const lang::EventObject& Event ) throw (RuntimeException);
            // XEventListener
            virtual void SAL_CALL disposing( const ::com::sun::star::lang::EventObject& Source ) throw (::com::sun::star::uno::RuntimeException);
        };

        // .............................................................................
        DatabaseDocumentLoader::DatabaseDocumentLoader( const comphelper::ComponentContext& _aContext )
        {
            acquire();
            try
            {
                m_xDesktop.set( _aContext.createComponent( (rtl::OUString)SERVICE_FRAME_DESKTOP ), UNO_QUERY_THROW );
                m_xDesktop->addTerminateListener( this );
            }
            catch( const Exception& )
            {
            	DBG_UNHANDLED_EXCEPTION();
            }
        }

        struct TerminateFunctor : ::std::unary_function<ODatabaseModelImpl* , void>
        {
            void operator()( const ODatabaseModelImpl* _pModelImpl ) const
            {
                try
                {
                    const Reference< XModel2> xModel( _pModelImpl ->getModel_noCreate(),UNO_QUERY_THROW );
                    if ( !xModel->getControllers()->hasMoreElements() )
                    {
                        Reference<util::XCloseable> xCloseable(xModel,UNO_QUERY_THROW);
                        xCloseable->close(sal_False);
                    } // if ( !xModel->getControllers()->hasMoreElements() )
                }
                catch(const CloseVetoException&)
                {
                    throw TerminationVetoException();
                }
            }
        };
        // .............................................................................
        void SAL_CALL DatabaseDocumentLoader::queryTermination( const lang::EventObject& /*Event*/ ) throw (TerminationVetoException, RuntimeException)
        {
            ::std::list< const ODatabaseModelImpl* > aCopy(m_aDatabaseDocuments);
            ::std::for_each(aCopy.begin(),aCopy.end(),TerminateFunctor());
        }

        // .............................................................................
        void SAL_CALL DatabaseDocumentLoader::notifyTermination( const lang::EventObject& /*Event*/ ) throw (RuntimeException)
        {
        }
        // .............................................................................
        void SAL_CALL DatabaseDocumentLoader::disposing( const lang::EventObject& /*Source*/ ) throw (RuntimeException)
        {
        }

//= ODatabaseContext
//==========================================================================
//--------------------------------------------------------------------------
ODatabaseContext::ODatabaseContext( const Reference< XComponentContext >& _rxContext )
    :DatabaseAccessContext_Base(m_aMutex)
    ,m_aContext( _rxContext )
    ,m_aContainerListeners(m_aMutex)
{
    m_pDatabaseDocumentLoader = new DatabaseDocumentLoader( m_aContext );
    ::basic::BasicManagerRepository::registerCreationListener( *this );

    osl_incrementInterlockedCount( &m_refCount );
    {
        m_xDBRegistrationAggregate.set( createDataSourceRegistrations( m_aContext ), UNO_SET_THROW );
        m_xDatabaseRegistrations.set( m_xDBRegistrationAggregate, UNO_QUERY_THROW );

        m_xDBRegistrationAggregate->setDelegator( *this );
    }
    osl_decrementInterlockedCount( &m_refCount );
}

//--------------------------------------------------------------------------
ODatabaseContext::~ODatabaseContext()
{
    ::basic::BasicManagerRepository::revokeCreationListener( *this );
    if ( m_pDatabaseDocumentLoader )
        m_pDatabaseDocumentLoader->release();

    m_xDBRegistrationAggregate->setDelegator( NULL );
    m_xDBRegistrationAggregate.clear();
    m_xDatabaseRegistrations.clear();
}

// Helper
//------------------------------------------------------------------------------
rtl::OUString ODatabaseContext::getImplementationName_static() throw( RuntimeException )

{
	return ::rtl::OUString(RTL_CONSTASCII_USTRINGPARAM("com.sun.star.comp.dba.ODatabaseContext"));
}

//------------------------------------------------------------------------------
Reference< XInterface > ODatabaseContext::Create(const Reference< XComponentContext >& _rxContext)
{
	return *( new ODatabaseContext( _rxContext ) );
}

//------------------------------------------------------------------------------
Sequence< rtl::OUString > ODatabaseContext::getSupportedServiceNames_static(void) throw( RuntimeException )
{
	Sequence< ::rtl::OUString > aSNS( 1 );
	aSNS[0] = SERVICE_SDB_DATABASECONTEXT;
	return aSNS;
}

// XServiceInfo
//------------------------------------------------------------------------------
rtl::OUString ODatabaseContext::getImplementationName(  ) throw(RuntimeException)
{
	return getImplementationName_static();
}

//------------------------------------------------------------------------------
sal_Bool ODatabaseContext::supportsService( const ::rtl::OUString& _rServiceName ) throw (RuntimeException)
{
	return ::comphelper::findValue(getSupportedServiceNames(), _rServiceName, sal_True).getLength() != 0;
}

//------------------------------------------------------------------------------
Sequence< ::rtl::OUString > ODatabaseContext::getSupportedServiceNames(  ) throw (RuntimeException)
{
	return getSupportedServiceNames_static();
}

//--------------------------------------------------------------------------
Reference< XInterface > ODatabaseContext::impl_createNewDataSource()
{
	::rtl::Reference<ODatabaseModelImpl> pImpl( new ODatabaseModelImpl( m_aContext.getLegacyServiceFactory(), *this ) );
    Reference< XDataSource > xDataSource( pImpl->getOrCreateDataSource() );

    return xDataSource.get();
}

//--------------------------------------------------------------------------
Reference< XInterface > SAL_CALL ODatabaseContext::createInstance(  ) throw (Exception, RuntimeException)
{
    // for convenience of the API user, we ensure the document is fully initialized (effectively: XLoadable::initNew
    // has been called at the DatabaseDocument).
    return impl_createNewDataSource();
}

//--------------------------------------------------------------------------
Reference< XInterface > SAL_CALL ODatabaseContext::createInstanceWithArguments( const Sequence< Any >& _rArguments ) throw (Exception, RuntimeException)
{
    ::comphelper::NamedValueCollection aArgs( _rArguments );
    ::rtl::OUString sURL = aArgs.getOrDefault( (::rtl::OUString)INFO_POOLURL, ::rtl::OUString() );

	Reference< XInterface > xDataSource;
    if ( sURL.getLength() )
        xDataSource = getObject( sURL );

    if ( !xDataSource.is() )
		xDataSource = impl_createNewDataSource();

	return xDataSource;
}
// DatabaseAccessContext_Base
//------------------------------------------------------------------------------
void ODatabaseContext::disposing()
{
	// notify our listener
	com::sun::star::lang::EventObject aDisposeEvent(static_cast< XContainer* >(this));
	m_aContainerListeners.disposeAndClear(aDisposeEvent);

	// dispose the data sources
	ObjectCache::iterator aEnd = m_aDatabaseObjects.end();
	for	(	ObjectCache::iterator	aIter = m_aDatabaseObjects.begin();
			aIter != aEnd;
			++aIter
		)
	{
		aIter->second->dispose();
	}
	m_aDatabaseObjects.clear();
}

// XNamingService
//------------------------------------------------------------------------------
Reference< XInterface >  ODatabaseContext::getRegisteredObject(const rtl::OUString& _rName) throw( Exception, RuntimeException )
{
	MutexGuard aGuard(m_aMutex);
	::connectivity::checkDisposed(DatabaseAccessContext_Base::rBHelper.bDisposed);

    ::rtl::OUString sURL( getDatabaseLocation( _rName ) );

	if ( !sURL.getLength() )
        // there is a registration for this name, but no URL
		throw IllegalArgumentException();

	// check if URL is already loaded
	Reference< XInterface > xExistent = getObject( sURL );
	if ( xExistent.is() )
		return xExistent;

	return loadObjectFromURL( _rName, sURL );
}
// -----------------------------------------------------------------------------
Reference< XInterface > ODatabaseContext::loadObjectFromURL(const ::rtl::OUString& _rName,const ::rtl::OUString& _sURL)
{
    INetURLObject aURL( _sURL );
    if ( aURL.GetProtocol() == INET_PROT_NOT_VALID )
        throw NoSuchElementException( _rName, *this );

	try
	{
		::ucbhelper::Content aContent( _sURL, NULL );
		if ( !aContent.isDocument() )
			throw InteractiveIOException(
                _sURL, *this, InteractionClassification_ERROR, IOErrorCode_NO_FILE
            );
	}
	catch ( const InteractiveIOException& e )
	{
        if  (   ( e.Code == IOErrorCode_NO_FILE )
            ||  ( e.Code == IOErrorCode_NOT_EXISTING )
            ||  ( e.Code == IOErrorCode_NOT_EXISTING_PATH )
            )
        {
            // #i40463# #i39187#
            String sErrorMessage( DBACORE_RESSTRING( RID_STR_FILE_DOES_NOT_EXIST ) );
            ::svt::OFileNotation aTransformer( _sURL );
		    sErrorMessage.SearchAndReplaceAscii( "$file$", aTransformer.get( ::svt::OFileNotation::N_SYSTEM ) );

            SQLException aError;
            aError.Message = sErrorMessage;

            throw WrappedTargetException( _sURL, *this, makeAny( aError ) );
        }
		throw WrappedTargetException( _sURL, *this, ::cppu::getCaughtException() );
	}
	catch( const Exception& )
	{
        throw WrappedTargetException( _sURL, *this, ::cppu::getCaughtException() );
	}

    OSL_ENSURE( m_aDatabaseObjects.find( _sURL ) == m_aDatabaseObjects.end(),
        "ODatabaseContext::loadObjectFromURL: not intended for already-cached objects!" );

    ::rtl::Reference< ODatabaseModelImpl > pModelImpl;
    {
		pModelImpl.set( new ODatabaseModelImpl( _rName, m_aContext.getLegacyServiceFactory(), *this ) );

	    Reference< XModel > xModel( pModelImpl->createNewModel_deliverOwnership( false ), UNO_SET_THROW );
        Reference< XLoadable > xLoad( xModel, UNO_QUERY_THROW );

        ::comphelper::NamedValueCollection aArgs;
        aArgs.put( "URL", _sURL );
        aArgs.put( "MacroExecutionMode", MacroExecMode::USE_CONFIG );
        aArgs.put( "InteractionHandler", m_aContext.createComponent( "com.sun.star.task.InteractionHandler" ) );

        Sequence< PropertyValue > aResource( aArgs.getPropertyValues() );
        xLoad->load( aResource );
        xModel->attachResource( _sURL, aResource );

        ::utl::CloseableComponent aEnsureClose( xModel );
    }

    setTransientProperties( _sURL, *pModelImpl );

    return pModelImpl->getOrCreateDataSource().get();
}
// -----------------------------------------------------------------------------
void ODatabaseContext::appendAtTerminateListener(const ODatabaseModelImpl& _rDataSourceModel)
{
    m_pDatabaseDocumentLoader->append(_rDataSourceModel);
}
// -----------------------------------------------------------------------------
void ODatabaseContext::removeFromTerminateListener(const ODatabaseModelImpl& _rDataSourceModel)
{
    m_pDatabaseDocumentLoader->remove(_rDataSourceModel);
}
// -----------------------------------------------------------------------------
void ODatabaseContext::setTransientProperties(const ::rtl::OUString& _sURL, ODatabaseModelImpl& _rDataSourceModel )
{
	if ( m_aDatasourceProperties.end() == m_aDatasourceProperties.find(_sURL) )
        return;
    try
    {
        ::rtl::OUString sAuthFailedPassword;
	    Reference< XPropertySet > xDSProps( _rDataSourceModel.getOrCreateDataSource(), UNO_QUERY_THROW );
		const Sequence< PropertyValue >& rSessionPersistentProps = m_aDatasourceProperties[_sURL];
		const PropertyValue* pProp = rSessionPersistentProps.getConstArray();
		const PropertyValue* pPropsEnd = rSessionPersistentProps.getConstArray() + rSessionPersistentProps.getLength();
        for ( ; pProp != pPropsEnd; ++pProp )
        {
            if ( pProp->Name.equalsAscii( "AuthFailedPassword" ) )
            {
                OSL_VERIFY( pProp->Value >>= sAuthFailedPassword );
            }
            else
            {
				xDSProps->setPropertyValue( pProp->Name, pProp->Value );
            }
		}

        _rDataSourceModel.m_sFailedPassword = sAuthFailedPassword;
    }
    catch( const Exception& )
    {
    	DBG_UNHANDLED_EXCEPTION();
    }
}

//------------------------------------------------------------------------------
void ODatabaseContext::registerObject(const rtl::OUString& _rName, const Reference< XInterface > & _rxObject) throw( Exception, RuntimeException )
{
	MutexGuard aGuard(m_aMutex);
	::connectivity::checkDisposed(DatabaseAccessContext_Base::rBHelper.bDisposed);

	if ( !_rName.getLength() )
		throw IllegalArgumentException( ::rtl::OUString(), *this, 1 );

    Reference< XDocumentDataSource > xDocDataSource( _rxObject, UNO_QUERY );
	Reference< XModel > xModel( xDocDataSource.is() ? xDocDataSource->getDatabaseDocument() : Reference< XOfficeDatabaseDocument >(), UNO_QUERY );
    if ( !xModel.is() )
		throw IllegalArgumentException( ::rtl::OUString(), *this, 2 );

	::rtl::OUString sURL = xModel->getURL();
	if ( !sURL.getLength() )
		throw IllegalArgumentException( DBACORE_RESSTRING( RID_STR_DATASOURCE_NOT_STORED ), *this, 2 );

    registerDatabaseLocation( _rName, sURL );

    ODatabaseSource::setName( xDocDataSource, _rName, ODatabaseSource::DBContextAccess() );

	// notify our container listeners
	ContainerEvent aEvent(static_cast<XContainer*>(this), makeAny(_rName), makeAny(_rxObject), Any());
    m_aContainerListeners.notifyEach( &XContainerListener::elementInserted, aEvent );
}

//------------------------------------------------------------------------------
void ODatabaseContext::storeTransientProperties( ODatabaseModelImpl& _rModelImpl)
{
    Reference< XPropertySet > xSource( _rModelImpl.getOrCreateDataSource(), UNO_QUERY );
    ::comphelper::NamedValueCollection aRememberProps;

	try
	{
		// get the info about the properties, check which ones are transient and not readonly
		Reference< XPropertySetInfo > xSetInfo;
		if (xSource.is())
			xSetInfo = xSource->getPropertySetInfo();
		Sequence< Property > aProperties;
		if (xSetInfo.is())
			aProperties = xSetInfo->getProperties();

		if (aProperties.getLength())
		{
			const Property* pProperties = aProperties.getConstArray();
			for ( sal_Int32 i=0; i<aProperties.getLength(); ++i, ++pProperties )
			{
				if	(	( ( pProperties->Attributes & PropertyAttribute::TRANSIENT) != 0 )
					&&	( ( pProperties->Attributes & PropertyAttribute::READONLY) == 0 )
					)
				{
					// found such a property
                    aRememberProps.put( pProperties->Name, xSource->getPropertyValue( pProperties->Name ) );
				}
			}
		}
	}
	catch ( const Exception& )
	{
        DBG_UNHANDLED_EXCEPTION();
	}

    // additionally, remember the "failed password", which is not available as property
    // #i86178# / 2008-02-19 / frank.schoenheit@sun.com
    aRememberProps.put( "AuthFailedPassword", _rModelImpl.m_sFailedPassword );

    ::rtl::OUString sDocumentURL( _rModelImpl.getURL() );
    if ( m_aDatabaseObjects.find( sDocumentURL ) != m_aDatabaseObjects.end() )
    {
	    m_aDatasourceProperties[ sDocumentURL ] = aRememberProps.getPropertyValues();
    }
    else if ( m_aDatabaseObjects.find( _rModelImpl.m_sName ) != m_aDatabaseObjects.end() )
    {
        OSL_ENSURE( false, "ODatabaseContext::storeTransientProperties: a database document register by name? This shouldn't happen anymore!" );
            // all the code should have been changed so that registration is by URL only
	    m_aDatasourceProperties[ _rModelImpl.m_sName ] = aRememberProps.getPropertyValues();
    }
    else
    {
        OSL_ENSURE( ( sDocumentURL.getLength() == 0 ) && ( _rModelImpl.m_sName.getLength() == 0 ),
            "ODatabaseContext::storeTransientProperties: a non-empty data source which I do not know?!" );
    }
}

//------------------------------------------------------------------------------
void SAL_CALL ODatabaseContext::addContainerListener( const Reference< XContainerListener >& _rxListener ) throw(RuntimeException)
{
	m_aContainerListeners.addInterface(_rxListener);
}

//------------------------------------------------------------------------------
void SAL_CALL ODatabaseContext::removeContainerListener( const Reference< XContainerListener >& _rxListener ) throw(RuntimeException)
{
	m_aContainerListeners.removeInterface(_rxListener);
}

//------------------------------------------------------------------------------
void ODatabaseContext::revokeObject(const rtl::OUString& _rName) throw( Exception, RuntimeException )
{
	ClearableMutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(DatabaseAccessContext_Base::rBHelper.bDisposed);

    ::rtl::OUString sURL = getDatabaseLocation( _rName );

    revokeDatabaseLocation( _rName );
        // will throw if something goes wrong

    if ( m_aDatabaseObjects.find( _rName ) != m_aDatabaseObjects.end() )
    {
        m_aDatasourceProperties[ sURL ] = m_aDatasourceProperties[ _rName ];
    }

	// check if URL is already loaded
	ObjectCacheIterator aExistent = m_aDatabaseObjects.find( sURL );
	if ( aExistent != m_aDatabaseObjects.end() )
		m_aDatabaseObjects.erase( aExistent );

	// notify our container listeners
	ContainerEvent aEvent( *this, makeAny( _rName ), Any(), Any() );
    aGuard.clear();
    m_aContainerListeners.notifyEach( &XContainerListener::elementRemoved, aEvent );
}

//------------------------------------------------------------------------------
::sal_Bool SAL_CALL ODatabaseContext::hasRegisteredDatabase( const ::rtl::OUString& _Name ) throw (IllegalArgumentException, RuntimeException)
{
    return m_xDatabaseRegistrations->hasRegisteredDatabase( _Name );
}

//------------------------------------------------------------------------------
Sequence< ::rtl::OUString > SAL_CALL ODatabaseContext::getRegistrationNames() throw (RuntimeException)
{
    return m_xDatabaseRegistrations->getRegistrationNames();
}

//------------------------------------------------------------------------------
::rtl::OUString SAL_CALL ODatabaseContext::getDatabaseLocation( const ::rtl::OUString& _Name ) throw (IllegalArgumentException, NoSuchElementException, RuntimeException)
{
    return m_xDatabaseRegistrations->getDatabaseLocation( _Name );
}

//------------------------------------------------------------------------------
void SAL_CALL ODatabaseContext::registerDatabaseLocation( const ::rtl::OUString& _Name, const ::rtl::OUString& _Location ) throw (IllegalArgumentException, ElementExistException, RuntimeException)
{
    m_xDatabaseRegistrations->registerDatabaseLocation( _Name, _Location );
}

//------------------------------------------------------------------------------
void SAL_CALL ODatabaseContext::revokeDatabaseLocation( const ::rtl::OUString& _Name ) throw (IllegalArgumentException, NoSuchElementException, IllegalAccessException, RuntimeException)
{
    m_xDatabaseRegistrations->revokeDatabaseLocation( _Name );
}

//------------------------------------------------------------------------------
void SAL_CALL ODatabaseContext::changeDatabaseLocation( const ::rtl::OUString& _Name, const ::rtl::OUString& _NewLocation ) throw (IllegalArgumentException, NoSuchElementException, IllegalAccessException, RuntimeException)
{
    m_xDatabaseRegistrations->changeDatabaseLocation( _Name, _NewLocation );
}

//------------------------------------------------------------------------------
::sal_Bool SAL_CALL ODatabaseContext::isDatabaseRegistrationReadOnly( const ::rtl::OUString& _Name ) throw (IllegalArgumentException, NoSuchElementException, RuntimeException)
{
    return m_xDatabaseRegistrations->isDatabaseRegistrationReadOnly( _Name );
}

//------------------------------------------------------------------------------
void SAL_CALL ODatabaseContext::addDatabaseRegistrationsListener( const Reference< XDatabaseRegistrationsListener >& _Listener ) throw (RuntimeException)
{
    m_xDatabaseRegistrations->addDatabaseRegistrationsListener( _Listener );
}

//------------------------------------------------------------------------------
void SAL_CALL ODatabaseContext::removeDatabaseRegistrationsListener( const Reference< XDatabaseRegistrationsListener >& _Listener ) throw (RuntimeException)
{
    m_xDatabaseRegistrations->removeDatabaseRegistrationsListener( _Listener );
}

// ::com::sun::star::container::XElementAccess
//------------------------------------------------------------------------------
Type ODatabaseContext::getElementType(  ) throw(RuntimeException)
{
	return::getCppuType(static_cast<Reference<XDataSource>*>(NULL));
}

//------------------------------------------------------------------------------
sal_Bool ODatabaseContext::hasElements(void) throw( RuntimeException )
{
	MutexGuard aGuard(m_aMutex);
	::connectivity::checkDisposed(DatabaseAccessContext_Base::rBHelper.bDisposed);

	return 0 != getElementNames().getLength();
}

// ::com::sun::star::container::XEnumerationAccess
//------------------------------------------------------------------------------
Reference< ::com::sun::star::container::XEnumeration >  ODatabaseContext::createEnumeration(void) throw( RuntimeException )
{
	MutexGuard aGuard(m_aMutex);
	return new ::comphelper::OEnumerationByName(static_cast<XNameAccess*>(this));
}

// ::com::sun::star::container::XNameAccess
//------------------------------------------------------------------------------
Any ODatabaseContext::getByName(const rtl::OUString& _rName) throw( NoSuchElementException,
														  WrappedTargetException, RuntimeException )
{
	MutexGuard aGuard(m_aMutex);
	::connectivity::checkDisposed(DatabaseAccessContext_Base::rBHelper.bDisposed);
	if ( !_rName.getLength() )
		throw NoSuchElementException(_rName, *this);

	try
	{
		Reference< XInterface > xExistent = getObject( _rName );
		if ( xExistent.is() )
			return makeAny( xExistent );

        // see whether this is an registered name
        ::rtl::OUString sURL;
        if ( hasRegisteredDatabase( _rName ) )
        {
            sURL = getDatabaseLocation( _rName );
            // is the object cached under its URL?
	        xExistent = getObject( sURL );
        }
        else
            // interpret the name as URL
            sURL = _rName;

        if ( !xExistent.is() )
		    // try to load this as URL
            xExistent = loadObjectFromURL( _rName, sURL );
		return makeAny( xExistent );
	}
	catch (NoSuchElementException&)
	{	// let these exceptions through
		throw;
	}
	catch (WrappedTargetException&)
	{	// let these exceptions through
		throw;
	}
	catch (RuntimeException&)
	{	// let these exceptions through
		throw;
	}
	catch (Exception& e)
	{	// exceptions other than the speciafied ones -> wrap
        Any aError = ::cppu::getCaughtException();
		throw WrappedTargetException(_rName, *this, aError );
	}
}

//------------------------------------------------------------------------------
Sequence< rtl::OUString > ODatabaseContext::getElementNames(void) throw( RuntimeException )
{
	MutexGuard aGuard(m_aMutex);
    ::connectivity::checkDisposed(DatabaseAccessContext_Base::rBHelper.bDisposed);

    return getRegistrationNames();
}

//------------------------------------------------------------------------------
sal_Bool ODatabaseContext::hasByName(const rtl::OUString& _rName) throw( RuntimeException )
{
	MutexGuard aGuard(m_aMutex);
	::connectivity::checkDisposed(DatabaseAccessContext_Base::rBHelper.bDisposed);

    return hasRegisteredDatabase( _rName );
}

// -----------------------------------------------------------------------------
Reference< XInterface > ODatabaseContext::getObject( const ::rtl::OUString& _rURL )
{
	ObjectCacheIterator aFind = m_aDatabaseObjects.find( _rURL );
	Reference< XInterface > xExistent;
	if ( aFind != m_aDatabaseObjects.end() )
		xExistent = aFind->second->getOrCreateDataSource();
	return xExistent;
}
// -----------------------------------------------------------------------------
void ODatabaseContext::registerDatabaseDocument( ODatabaseModelImpl& _rModelImpl )
{
    ::rtl::OUString sURL( _rModelImpl.getURL() );
#if OSL_DEBUG_LEVEL > 1
    OSL_TRACE( "DatabaseContext: registering %s", ::rtl::OUStringToOString( sURL, RTL_TEXTENCODING_UTF8 ).getStr() );
#endif
	if ( m_aDatabaseObjects.find( sURL ) == m_aDatabaseObjects.end() )
	{
		m_aDatabaseObjects[ sURL ] = &_rModelImpl;
        setTransientProperties( sURL, _rModelImpl );
	}
    else
        OSL_ENSURE( false, "ODatabaseContext::registerDatabaseDocument: already have an object registered for this URL!" );
}
// -----------------------------------------------------------------------------
void ODatabaseContext::revokeDatabaseDocument( const ODatabaseModelImpl& _rModelImpl )
{
    ::rtl::OUString sURL( _rModelImpl.getURL() );
#if OSL_DEBUG_LEVEL > 1
    OSL_TRACE( "DatabaseContext: deregistering %s", ::rtl::OUStringToOString( sURL, RTL_TEXTENCODING_UTF8 ).getStr() );
#endif
	m_aDatabaseObjects.erase( sURL );
}
// -----------------------------------------------------------------------------
void ODatabaseContext::databaseDocumentURLChange( const ::rtl::OUString& _rOldURL, const ::rtl::OUString& _rNewURL )
{
#if OSL_DEBUG_LEVEL > 1
    OSL_TRACE( "DatabaseContext: changing registration from %s to %s",
        ::rtl::OUStringToOString( _rOldURL, RTL_TEXTENCODING_UTF8 ).getStr(),
        ::rtl::OUStringToOString( _rNewURL, RTL_TEXTENCODING_UTF8 ).getStr() );
#endif
    ObjectCache::iterator oldPos = m_aDatabaseObjects.find( _rOldURL );
    ENSURE_OR_THROW( oldPos != m_aDatabaseObjects.end(), "illegal old database document URL" );
    ObjectCache::iterator newPos = m_aDatabaseObjects.find( _rNewURL );
    ENSURE_OR_THROW( newPos == m_aDatabaseObjects.end(), "illegal new database document URL" );

    m_aDatabaseObjects[ _rNewURL ] = oldPos->second;
    m_aDatabaseObjects.erase( oldPos );
}
// -----------------------------------------------------------------------------
sal_Int64 SAL_CALL ODatabaseContext::getSomething( const Sequence< sal_Int8 >& rId ) throw(RuntimeException)
{
	if (rId.getLength() == 16 && 0 == rtl_compareMemory(getUnoTunnelImplementationId().getConstArray(),  rId.getConstArray(), 16 ) )
		return reinterpret_cast<sal_Int64>(this);

	return 0;
}
// -----------------------------------------------------------------------------
Sequence< sal_Int8 > ODatabaseContext::getUnoTunnelImplementationId()
{
	static ::cppu::OImplementationId * pId = 0;
	if (! pId)
	{
		::osl::MutexGuard aGuard( ::osl::Mutex::getGlobalMutex() );
		if (! pId)
		{
			static ::cppu::OImplementationId aId;
			pId = &aId;
		}
	}
	return pId->getImplementationId();
}

// -----------------------------------------------------------------------------
void ODatabaseContext::onBasicManagerCreated( const Reference< XModel >& _rxForDocument, BasicManager& _rBasicManager )
{
    // if it's a database document ...
    Reference< XOfficeDatabaseDocument > xDatabaseDocument( _rxForDocument, UNO_QUERY );
    // ... or a sub document of a database document ...
    if ( !xDatabaseDocument.is() )
    {
        Reference< XChild > xDocAsChild( _rxForDocument, UNO_QUERY );
        if ( xDocAsChild.is() )
            xDatabaseDocument.set( xDocAsChild->getParent(), UNO_QUERY );
    }

    // ... whose BasicManager has just been created, then add the global DatabaseDocument variable to its scope.
    if ( xDatabaseDocument.is() )
        _rBasicManager.SetGlobalUNOConstant( "ThisDatabaseDocument", makeAny( xDatabaseDocument ) );
}

//........................................................................
}	// namespace dbaccess
//........................................................................
