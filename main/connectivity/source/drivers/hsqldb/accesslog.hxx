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



#ifndef CONNECTIVITY_HSQLDB_ACCESSLOG_HXX
#define CONNECTIVITY_HSQLDB_ACCESSLOG_HXX

#ifdef HSQLDB_DBG

#include <stdio.h>
#include <jni.h>
#include <rtl/ustring.hxx>
#include <rtl/string.hxx>

namespace connectivity { namespace hsqldb
{
    class LogFile
    {
    private:
        ::rtl::OUString     m_sFileName;

    public:
        LogFile( JNIEnv* env, jstring streamName, const sal_Char* _pAsciiSuffix );

    public:
                void    writeString( const sal_Char* _pString, bool _bEndLine = true );
                void    create() { getLogFile(); }
        virtual void    close();

    protected:
        FILE*&  getLogFile();
    };

    class OperationLogFile : public LogFile
    {
    public:
        OperationLogFile( JNIEnv* env, jstring streamName, const sal_Char* _pAsciiSuffix )
            :LogFile( env, streamName, ( ::rtl::OString( _pAsciiSuffix ) += ".op" ).getStr() )
        {
        }

        void logOperation( const sal_Char* _pOp )
        {
            writeString( _pOp, true );
        }

        void logOperation( const sal_Char* _pOp, jlong _nLongArg )
        {
            ::rtl::OString sLine( _pOp );
            sLine += "( ";
            sLine += ::rtl::OString::valueOf( _nLongArg );
            sLine += " )";
            writeString( sLine.getStr(), true );
        }

        void logReturn( jlong _nRetVal )
        {
            ::rtl::OString sLine( " -> " );
            sLine += ::rtl::OString::valueOf( _nRetVal );
            writeString( sLine.getStr(), true );
        }

        void logReturn( jint _nRetVal )
        {
            ::rtl::OString sLine( " -> " );
            sLine += ::rtl::OString::valueOf( _nRetVal );
            writeString( sLine.getStr(), true );
        }

        virtual void close()
        {
            writeString( "-------------------------------", true );
            writeString( "", true );
            LogFile::close();
        }
    };

    class DataLogFile : public LogFile
    {
    public:
        DataLogFile( JNIEnv* env, jstring streamName, const sal_Char* _pAsciiSuffix )
            :LogFile( env, streamName, _pAsciiSuffix )
        {
        }

        void write( jint value )
        {
			fputc( value, getLogFile() );
            fflush( getLogFile() );
        }

        void write( const sal_Int8* buffer, sal_Int32 bytesRead )
        {
			fwrite( buffer, sizeof(sal_Int8), bytesRead, getLogFile() );
            fflush( getLogFile() );
        }

        sal_Int64 seek( sal_Int64 pos )
        {
            FILE* pFile = getLogFile();
            fseek( pFile, 0, SEEK_END );
            if ( ftell( pFile ) < pos )
            {
                sal_Int8 filler( 0 );
                while ( ftell( pFile ) < pos )
                    fwrite( &filler, sizeof( sal_Int8 ), 1, pFile );
                fflush( pFile );
            }
            fseek( pFile, pos, SEEK_SET );
            return ftell( pFile );
        }

        sal_Int64 tell()
        {
            return ftell( getLogFile() );
        }
    };

} }
#endif

#endif // CONNECTIVITY_HSQLDB_ACCESSLOG_HXX
