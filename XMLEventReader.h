/* Copyright (C) 2017-2018 Stephan Kreutzer
 *
 * This file is part of CppStAX.
 *
 * CppStAX is free software: you can redistribute it and/or modify it under
 * the terms of the GNU Affero General Public License version 3 or any later
 * version of the license, as published by the Free Software Foundation.
 *
 * CppStAX is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU Affero General Public License 3 for more details.
 *
 * You should have received a copy of the GNU Affero General Public License 3
 * along with CppStAX. If not, see <http://www.gnu.org/licenses/>.
 */
/**
 * @file $/XMLEventReader.h
 * @author Stephan Kreutzer
 * @since 2017-08-24
 */

#ifndef _CPPSTAX_XMLEVENTREADER_H
#define _CPPSTAX_XMLEVENTREADER_H

#include "XMLEvent.h"
#include "Attribute.h"
#include <istream>
#include <locale>
#include <memory>
#include <queue>
#include <map>

namespace cppstax
{

class XMLEventReader
{
public:
    XMLEventReader(std::istream& aStream);
    ~XMLEventReader();

    bool hasNext();
    std::unique_ptr<XMLEvent> nextEvent();

protected:
    bool HandleTag();
    bool HandleTagStart(const char& cFirstByte);
    bool HandleTagEnd();
    bool HandleText(const char& cFirstByte);
    bool HandleProcessingInstruction();
    bool HandleProcessingInstructionTarget(std::unique_ptr<std::string>& pTarget);
    bool HandleMarkupDeclaration();
    bool HandleComment();
    bool HandleAttributes(const char& cFirstByte, std::unique_ptr<std::list<std::unique_ptr<Attribute>>>& pAttributes);
    bool HandleAttributeName(const char& cFirstByte, std::unique_ptr<QName>& pName);
    bool HandleAttributeValue(std::unique_ptr<std::string>& pValue);

protected:
    void ResolveEntity(std::unique_ptr<std::string>& pResolvedText);
    char ConsumeWhitespace();

protected:
    std::istream& m_aStream;
    std::locale m_aLocale;
    bool m_bHasNextCalled;
    std::queue<std::unique_ptr<XMLEvent>> m_aEvents;
    std::map<std::string, std::string> m_aEntityReplacementDictionary;

};

}

#endif
