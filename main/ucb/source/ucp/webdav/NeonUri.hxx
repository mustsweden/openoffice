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


#ifndef _NEONURI_HXX_
#define _NEONURI_HXX_

#include <ne_uri.h>
#include <rtl/ustring.hxx>
#include <DAVException.hxx>

namespace webdav_ucp
{

#define DEFAULT_HTTP_PORT       80
#define DEFAULT_HTTPS_PORT      443
#define DEFAULT_FTP_PORT        21

// -------------------------------------------------------------------
// NeonUri
// A URI implementation for use with the neon/expat library
// -------------------------------------------------------------------
class NeonUri
{
	private:
		::rtl::OUString	mURI;
		::rtl::OUString	mScheme;
		::rtl::OUString	mUserInfo;
		::rtl::OUString	mHostName;
		sal_Int32		mPort;
		::rtl::OUString	mPath;

        void init( const rtl::OString & rUri, const ne_uri * pUri );
		void calculateURI ();

	public:
        NeonUri( const ::rtl::OUString & inUri ) throw ( DAVException );
        NeonUri( const ne_uri * inUri ) throw ( DAVException );
		~NeonUri( );

        bool operator== ( const NeonUri & rOther ) const;
        bool operator!= ( const NeonUri & rOther ) const
        { return !operator==( rOther ); }

		const ::rtl::OUString & GetURI( void ) const
											{ return mURI; };
		const ::rtl::OUString & GetScheme( void ) const
											{ return mScheme; };
		const ::rtl::OUString & GetUserInfo( void ) const
											{ return mUserInfo; };
		const ::rtl::OUString & GetHost( void ) const
											{ return mHostName; };
		sal_Int32		GetPort( void )		const
											{ return mPort; };
		const ::rtl::OUString &		GetPath( void )	const
											{ return mPath; };

		::rtl::OUString GetPathBaseName ( void ) const;

		::rtl::OUString GetPathBaseNameUnescaped ( void ) const;

		void SetScheme (const ::rtl::OUString& scheme)
			{ mScheme = scheme; calculateURI (); };

		void AppendPath (const ::rtl::OUString& rPath);

		static ::rtl::OUString escapeSegment( const ::rtl::OUString& segment );
		static ::rtl::OUString unescape( const ::rtl::OUString& string );

        // "host:port", omit ":port" for port 80 and 443
        static rtl::OUString makeConnectionEndPointString(
                                        const rtl::OUString & rHostName,
                                        int nPort );
        rtl::OUString makeConnectionEndPointString() const
        { return makeConnectionEndPointString( GetHost(), GetPort() ); }
};

} // namespace webdav_ucp

#endif // _NEONURI_HXX_
