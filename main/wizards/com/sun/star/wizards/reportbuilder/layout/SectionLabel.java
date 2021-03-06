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


package com.sun.star.wizards.reportbuilder.layout;

import com.sun.star.awt.FontDescriptor;
import com.sun.star.report.XFixedText;
// import com.sun.star.wizards.common.PropertySetHelper;
/**
 *
 * @author ll93751
 */
public class SectionLabel extends SectionObject
{

    protected SectionLabel(XFixedText _aFixedText)
    {
        m_aParentObject = _aFixedText;
    // We would like to know, what properties are in this object.
//        PropertySetHelper aHelper = new PropertySetHelper(_aFixedText);
//        aHelper.showProperties();
    }

    public static SectionObject create(XFixedText _aFixedText)
    {
        return new SectionLabel(_aFixedText);
    }

    /**
     * Return the current FontDescriptor
     * @return
     */
    public FontDescriptor getFontDescriptor()
    {
        FontDescriptor a = null;
        try
        {
            final XFixedText aLabel = (XFixedText) getParent();
            a = aLabel.getFontDescriptor();
        }
        catch (com.sun.star.beans.UnknownPropertyException e)
        {
        }
        return a;
    }
}


