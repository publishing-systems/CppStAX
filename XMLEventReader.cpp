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
 * @file $/XMLEventReader.cpp
 * @todo Check well-formedness: check if start and end tags match by having a
 *     stack of the current XPath tree location/level/hierarchy/position, and
 *     if only one root element, and if starting with an element, etc.
 * @author Stephan Kreutzer
 * @since 2017-08-24
  */

#include "XMLEventReader.h"
#include "StartElement.h"
#include "EndElement.h"
#include "Characters.h"
#include "Comment.h"
#include "QName.h"
#include "Attribute.h"
#include <string>
#include <memory>
#include <sstream>
#include <iomanip>

namespace cppstax
{

XMLEventReader::XMLEventReader(std::istream& aStream):
  m_aStream(aStream),
  m_bHasNextCalled(false)
{
    m_aEntityReplacementDictionary.insert(std::pair<std::string, std::string>("amp", "&"));
    m_aEntityReplacementDictionary.insert(std::pair<std::string, std::string>("lt", "<"));
    m_aEntityReplacementDictionary.insert(std::pair<std::string, std::string>("gt", ">"));
    m_aEntityReplacementDictionary.insert(std::pair<std::string, std::string>("apos", "'"));
    m_aEntityReplacementDictionary.insert(std::pair<std::string, std::string>("quot", "\""));

    /** @todo Load more from a catalogue, which itself is written in XML and needs to be read
      * in here by another local XMLEventReader object, containing mappings from entity to
      * replacement characters. No need to deal with DTDs as they're non-XML, and extracting
      * those mappings from a DTD can be a separate program or manual procedure. */
}

XMLEventReader::~XMLEventReader()
{

}

bool XMLEventReader::hasNext()
{
    if (m_aEvents.size() > 0)
    {
        return true;
    }

    if (m_bHasNextCalled == true)
    {
        return false;
    }
    else
    {
        m_bHasNextCalled = true;
    }

    char cByte = '\0';
    m_aStream.get(cByte);

    if (m_aStream.eof() == true)
    {
        return false;
    }

    if (m_aStream.bad() == true)
    {
        throw new std::runtime_error("Stream is bad.");
    }

    if (cByte == '<')
    {
        return HandleTag();
    }
    else
    {
        return HandleText(cByte);
    }
}

std::unique_ptr<XMLEvent> XMLEventReader::nextEvent()
{
    if (m_aEvents.size() <= 0 &&
        m_bHasNextCalled == false)
    {
        if (hasNext() != true)
        {
            throw new std::logic_error("Attempted XMLEventReader::nextEvent() while there isn't one instead of checking XMLEventReader::hasNext() first.");
        }
    }

    m_bHasNextCalled = false;

    if (m_aEvents.size() <= 0)
    {
        throw new std::logic_error("XMLEventReader::nextEvent() while there isn't one, ignoring XMLEventReader::hasNext() == false.");
    }

    std::unique_ptr<XMLEvent> pEvent = std::move(m_aEvents.front());
    m_aEvents.pop();

    return std::move(pEvent);
}

bool XMLEventReader::HandleTag()
{
    char cByte = '\0';
    m_aStream.get(cByte);

    if (m_aStream.eof() == true)
    {
        throw new std::runtime_error("Tag incomplete.");
    }

    if (m_aStream.bad() == true)
    {
        throw new std::runtime_error("Stream is bad.");
    }

    if (cByte == '?')
    {
        if (HandleProcessingInstruction() == true)
        {
            return true;
        }
        else
        {
            m_bHasNextCalled = false;
            return hasNext();
        }
    }
    else if (cByte == '/')
    {
        return HandleTagEnd();
    }
    else if (cByte == '!')
    {
        return HandleMarkupDeclaration();
    }
    else if (std::isalpha(cByte, m_aLocale) == true ||
             cByte == '_')
    {
        return HandleTagStart(cByte);
    }
    else
    {
        std::stringstream aMessage;
        aMessage << "Unknown byte '" << cByte << "' within element.";
        throw new std::runtime_error(aMessage.str());
    }
}

bool XMLEventReader::HandleTagStart(const char& cFirstByte)
{
    char cByte = '\0';
    m_aStream.get(cByte);

    if (m_aStream.eof() == true)
    {
        throw new std::runtime_error("Tag start incomplete.");
    }

    if (m_aStream.bad() == true)
    {
        throw new std::runtime_error("Stream is bad.");
    }

    std::unique_ptr<std::string> pNamePrefix(nullptr);
    std::unique_ptr<std::string> pNameLocalPart(new std::string());
    std::unique_ptr<std::list<std::unique_ptr<Attribute>>> pAttributes(new std::list<std::unique_ptr<Attribute>>);

    pNameLocalPart->push_back(cFirstByte);

    do
    {
        /** @todo Check, if special characters appear at the very start where not allowed.
          * This might apply for the namespace prefix as well as for the element name. */

        if (cByte == ':')
        {
            if (pNamePrefix != nullptr)
            {
                throw new std::runtime_error("There can't be two prefixes in element name.");
            }

            pNamePrefix = std::move(pNameLocalPart);
            pNameLocalPart.reset(new std::string());
        }
        else if (cByte == '>')
        {
            if (pNamePrefix == nullptr)
            {
                pNamePrefix.reset(new std::string);
            }

            std::unique_ptr<QName> pName(new QName("", *pNameLocalPart, *pNamePrefix));
            std::unique_ptr<StartElement> pStartElement(new StartElement(std::move(pName), std::move(pAttributes)));
            std::unique_ptr<XMLEvent> pEvent(new XMLEvent(std::move(pStartElement),
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr));
            m_aEvents.push(std::move(pEvent));
            break;
        }
        else if (cByte == '/')
        {
            m_aStream.get(cByte);

            if (m_aStream.eof() == true)
            {
                throw new std::runtime_error("Tag start incomplete.");
            }

            if (m_aStream.bad() == true)
            {
                throw new std::runtime_error("Stream is bad.");
            }

            if (cByte != '>')
            {
                throw new std::runtime_error("Empty start + end tag end without closing '>'.");
            }

            if (pNamePrefix == nullptr)
            {
                pNamePrefix.reset(new std::string);
            }

            std::unique_ptr<QName> pName(new QName("", *pNameLocalPart, *pNamePrefix));
            std::unique_ptr<StartElement> pStartElement(new StartElement(std::move(pName), std::move(pAttributes)));
            std::unique_ptr<XMLEvent> pEvent(new XMLEvent(std::move(pStartElement),
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr));
            m_aEvents.push(std::move(pEvent));

            pName = std::unique_ptr<QName>(new QName("", *pNameLocalPart, *pNamePrefix));
            std::unique_ptr<EndElement> pEndElement(new EndElement(std::move(pName)));
            pEvent = std::unique_ptr<XMLEvent>(new XMLEvent(nullptr,
                                                            std::move(pEndElement),
                                                            nullptr,
                                                            nullptr,
                                                            nullptr));
            m_aEvents.push(std::move(pEvent));

            break;
        }
        else if (std::isspace(cByte, m_aLocale) != 0)
        {
            if (pNameLocalPart->length() <= 0)
            {
                throw new std::runtime_error("Start tag name begins with whitespace.");
            }

            while (true)
            {
                m_aStream.get(cByte);

                if (m_aStream.eof() == true)
                {
                    throw new std::runtime_error("Tag start incomplete.");
                }

                if (m_aStream.bad() == true)
                {
                    throw new std::runtime_error("Stream is bad.");
                }

                if (cByte == '>')
                {
                    break;
                }
                else if (std::isspace(cByte, m_aLocale) != 0)
                {
                    // Ignore/consume.
                }
                else if (cByte == '/')
                {
                    m_aStream.unget();

                    if (m_aStream.bad() == true)
                    {
                        throw new std::runtime_error("Stream is bad.");
                    }

                    break;
                }
                else
                {
                    HandleAttributes(cByte, pAttributes);
                    break;
                }
            }
        }
        else if (std::isalnum(cByte, m_aLocale) == true ||
                 cByte == '-' ||
                 cByte == '_' ||
                 cByte == '.')
        {
            pNameLocalPart->push_back(cByte);
        }
        else
        {
            int nByte(cByte);
            std::stringstream strMessage;
            strMessage << "Character '" << cByte << "' (0x"
                       << std::hex << std::uppercase << nByte << std::nouppercase << std::dec
                       << ") not supported in a start tag name.";
            throw new std::runtime_error(strMessage.str());
        }

        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("Tag start incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

    } while (true);

    return true;
}

bool XMLEventReader::HandleTagEnd()
{
    char cByte = '\0';
    m_aStream.get(cByte);

    if (m_aStream.eof() == true)
    {
        throw new std::runtime_error("Tag end incomplete.");
    }

    if (m_aStream.bad() == true)
    {
        throw new std::runtime_error("Stream is bad.");
    }

    std::unique_ptr<std::string> pNamePrefix(nullptr);
    std::unique_ptr<std::string> pNameLocalPart(new std::string());

    // No validity check for the XML element name is needed
    // if end tags are compared to start tags and the start
    // tags were already checked.

    bool bComplete = false;

    do
    {
        if (cByte == ':')
        {
            if (pNamePrefix != nullptr)
            {
                throw new std::runtime_error("There can't be two prefixes in the element name.");
            }

            pNamePrefix = std::move(pNameLocalPart);
            pNameLocalPart.reset(new std::string());
        }
        else if (cByte == '>')
        {
            if (pNamePrefix == nullptr)
            {
                pNamePrefix.reset(new std::string);
            }

            std::unique_ptr<QName> pName(new QName("", *pNameLocalPart, *pNamePrefix));
            std::unique_ptr<EndElement> pEndElement(new EndElement(std::move(pName)));
            std::unique_ptr<XMLEvent> pEvent(new XMLEvent(nullptr,
                                                          std::move(pEndElement),
                                                          nullptr,
                                                          nullptr,
                                                          nullptr));
            m_aEvents.push(std::move(pEvent));

            bComplete = true;
            break;
        }
        else if (std::isalnum(cByte, m_aLocale) == true ||
                 cByte == '-' ||
                 cByte == '_' ||
                 cByte == '.')
        {
            pNameLocalPart->push_back(cByte);
        }
        else
        {
            int nByte(cByte);
            std::stringstream strMessage;
            strMessage << "Character '" << cByte << "' (0x"
                       << std::hex << std::uppercase << nByte << std::nouppercase << std::dec
                       << ") not supported in an end tag name.";
            throw new std::runtime_error(strMessage.str());
        }

        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("End tag incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

    } while (true);

    if (bComplete != true)
    {
        throw new std::runtime_error("End tag incomplete.");
    }

    return true;
}

bool XMLEventReader::HandleText(const char& cFirstByte)
{
    std::unique_ptr<std::string> pData(new std::string);

    if (cFirstByte == '&')
    {
        std::unique_ptr<std::string> pResolvedText = nullptr;

        ResolveEntity(pResolvedText);
        pData->append(*pResolvedText);
    }
    else
    {
        pData->push_back(cFirstByte);
    }

    char cByte = '\0';

    while (true)
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            break;
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (cByte == '<')
        {
            m_aStream.unget();

            if (m_aStream.bad() == true)
            {
                throw new std::runtime_error("Stream is bad.");
            }

            break;
        }
        else if (cByte == '&')
        {
            std::unique_ptr<std::string> pResolvedText = nullptr;

            ResolveEntity(pResolvedText);
            pData->append(*pResolvedText);
        }
        else
        {
            pData->push_back(cByte);
        }
    }

    std::unique_ptr<Characters> pCharacters(new Characters(std::move(pData)));
    std::unique_ptr<XMLEvent> pEvent(new XMLEvent(nullptr,
                                                  nullptr,
                                                  std::move(pCharacters),
                                                  nullptr,
                                                  nullptr));
    m_aEvents.push(std::move(pEvent));

    return true;
}

bool XMLEventReader::HandleProcessingInstruction()
{
    std::unique_ptr<std::string> pTarget = nullptr;

    HandleProcessingInstructionTarget(pTarget);

    if (pTarget == nullptr)
    {
        throw new std::runtime_error("Processing instruction without target name.");
    }

    if (pTarget->length() == 3)
    {
        if ((pTarget->at(0) == 'x' ||
             pTarget->at(0) == 'X') &&
            (pTarget->at(1) == 'm' ||
             pTarget->at(1) == 'M') &&
            (pTarget->at(2) == 'l' ||
             pTarget->at(2) == 'L'))
        {
            /** @todo This should read the XML declaration instructions instead
              * of just consuming/ignoring it. */

            char cByte('\0');
            int nMatchCount = 0;

            while (nMatchCount < 2)
            {
                m_aStream.get(cByte);

                if (m_aStream.eof() == true)
                {
                    throw new std::runtime_error("XML declaration incomplete.");
                }

                if (m_aStream.bad() == true)
                {
                    throw new std::runtime_error("Stream is bad.");
                }

                if (cByte == '?' &&
                    nMatchCount <= 0)
                {
                    nMatchCount++;
                }
                else if (cByte == '>' &&
                        nMatchCount <= 1)
                {
                    return false;
                }
            }

            return false;
        }
    }

    std::unique_ptr<std::string> pData(new std::string);
    char cByte('\0');
    int nMatchCount = 0;

    while (nMatchCount < 2)
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("Processing instruction data incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (cByte == '?' &&
            nMatchCount <= 0)
        {
            nMatchCount++;
        }
        else if (cByte == '>' &&
                 nMatchCount <= 1)
        {
            //nMatchCount++;

            std::unique_ptr<ProcessingInstruction> pProcessingInstruction(new ProcessingInstruction(std::move(pTarget), std::move(pData)));
            std::unique_ptr<XMLEvent> pEvent(new XMLEvent(nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          nullptr,
                                                          std::move(pProcessingInstruction)));
            m_aEvents.push(std::move(pEvent));

            return true;
        }
        else
        {
            if (nMatchCount > 0)
            {
                pData->push_back('?');
            }

            nMatchCount = 0;

            pData->push_back(cByte);
        }
    }

    return false;
}

bool XMLEventReader::HandleProcessingInstructionTarget(std::unique_ptr<std::string>& pTarget)
{
    std::unique_ptr<std::string> pName = nullptr;
    char cByte('\0');
    int nMatchCount = 0;

    while (nMatchCount < 2)
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("Processing instruction target name incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (cByte == '?' &&
            nMatchCount <= 0)
        {
            nMatchCount++;
        }
        else if (cByte == '>' &&
                 nMatchCount <= 1)
        {
            throw new std::runtime_error("Processing instruction ended before processing instruction target name could be read.");
        }
        else if (std::isspace(cByte, m_aLocale) != 0)
        {
            if (pName == nullptr)
            {
                throw new std::runtime_error("Processing instruction without target name.");
            }

            pTarget = std::move(pName);

            return true;
        }
        else
        {
            if (nMatchCount > 0)
            {
                throw new std::runtime_error("Processing instruction target name interrupted by '?'.");
            }

            if (pName == nullptr)
            {
                if (std::isalpha(cByte, m_aLocale) != true)
                {
                    int nByte(cByte);
                    std::stringstream strMessage;
                    strMessage << "Character '" << cByte << "' (0x"
                                << std::hex << std::uppercase << nByte << std::nouppercase << std::dec
                                << ") not supported as first character of an processing instruction target name.";
                    throw new std::runtime_error(strMessage.str());
                }

                pName = std::unique_ptr<std::string>(new std::string);
            }

            pName->push_back(cByte);
        }
    }

    return false;
}

bool XMLEventReader::HandleMarkupDeclaration()
{
    char cByte = '\0';
    m_aStream.get(cByte);

    if (m_aStream.eof() == true)
    {
        throw new std::runtime_error("Markup declaration incomplete.");
    }

    if (m_aStream.bad() == true)
    {
        throw new std::runtime_error("Steam is bad.");
    }

    if (cByte == '-')
    {
        return HandleComment();
    }
    else
    {
        throw new std::runtime_error("Markup declaration type not implemented yet.");
    }

    return true;
}

bool XMLEventReader::HandleComment()
{
    char cByte = '\0';
    m_aStream.get(cByte);

    if (m_aStream.eof() == true)
    {
        throw new std::runtime_error("Comment incomplete.");
    }

    if (m_aStream.bad() == true)
    {
        throw new std::runtime_error("Stream is bad.");
    }

    if (cByte != '-')
    {
        throw new std::runtime_error("Comment malformed.");
    }

    std::unique_ptr<std::string> pData(new std::string);

    unsigned int nMatchCount = 0;
    const unsigned int END_SEQUENCE_LENGTH = 3;
    char cEndSequence[END_SEQUENCE_LENGTH] = { '-', '-', '>' };

    do
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("Comment incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (cByte == cEndSequence[nMatchCount])
        {
            if (nMatchCount + 1 < END_SEQUENCE_LENGTH)
            {
                ++nMatchCount;
            }
            else
            {
                std::unique_ptr<Comment> pComment(new Comment(std::move(pData)));
                std::unique_ptr<XMLEvent> pEvent(new XMLEvent(nullptr,
                                                              nullptr,
                                                              nullptr,
                                                              std::move(pComment),
                                                              nullptr));
                m_aEvents.push(std::move(pEvent));

                break;
            }
        }
        else
        {
            if (nMatchCount > 0)
            {
                // Instead of strncpy() and C-style char*.
                for (unsigned int i = 0; i < nMatchCount; i++)
                {
                    pData->push_back(cEndSequence[i]);
                }

                pData->push_back(cByte);
                nMatchCount = 0;
            }
            else
            {
                pData->push_back(cByte);
            }
        }

    } while (true);

    return true;
}

bool XMLEventReader::HandleAttributes(const char& cFirstByte, std::unique_ptr<std::list<std::unique_ptr<Attribute>>>& pAttributes)
{
    if (pAttributes == nullptr)
    {
        throw new std::invalid_argument("nullptr passed.");
    }

    std::unique_ptr<QName> pAttributeName(nullptr);
    std::unique_ptr<std::string> pAttributeValue(nullptr);

    HandleAttributeName(cFirstByte, pAttributeName);
    HandleAttributeValue(pAttributeValue);

    pAttributes->push_back(std::unique_ptr<Attribute>(new Attribute(std::move(pAttributeName), std::move(pAttributeValue))));

    char cByte('\0');

    do
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("Tag start incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (cByte == '>')
        {
            // Not part of the attributes any more and indicator for outer
            // methods to complete the StartElement.
            m_aStream.unget();

            break;
        }
        else if (cByte == '/')
        {
            m_aStream.get(cByte);

            if (m_aStream.eof() == true)
            {
                throw new std::runtime_error("Tag start incomplete.");
            }

            if (m_aStream.bad() == true)
            {
                throw new std::runtime_error("Stream is bad.");
            }

            if (cByte != '>')
            {
                throw new std::runtime_error("Empty start + end tag end without closing '>'.");
            }

            m_aStream.unget();

            if (m_aStream.bad() == true)
            {
                throw new std::runtime_error("Stream is bad.");
            }

            m_aStream.unget();

            if (m_aStream.bad() == true)
            {
                throw new std::runtime_error("Stream is bad.");
            }

            break;
        }
        else if (std::isspace(cByte, m_aLocale) != 0)
        {
            // Ignore/consume.
            continue;
        }
        else
        {
            HandleAttributeName(cByte, pAttributeName);
            HandleAttributeValue(pAttributeValue);

            pAttributes->push_back(std::unique_ptr<Attribute>(new Attribute(std::move(pAttributeName), std::move(pAttributeValue))));
        }

    } while (true);

    return true;
}

bool XMLEventReader::HandleAttributeName(const char& cFirstByte, std::unique_ptr<QName>& pName)
{
    std::unique_ptr<std::string> pNamePrefix(nullptr);
    std::unique_ptr<std::string> pNameLocalPart(new std::string());

    char cByte(cFirstByte);

    if (cByte == ':')
    {
        pNamePrefix.reset(new std::string());
    }
    else if (std::isalnum(cByte, m_aLocale) == true ||
             cByte == '_')
    {
        pNameLocalPart->push_back(cByte);
    }
    else
    {
        int nByte(cByte);
        std::stringstream strMessage;
        strMessage << "Character '" << cByte << "' (0x"
                    << std::hex << std::uppercase << nByte << std::nouppercase << std::dec
                    << ") not supported as first character of an attribute name.";
        throw new std::runtime_error(strMessage.str());
    }

    do
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("Attribute name incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (cByte == ':')
        {
            if (pNamePrefix != nullptr)
            {
                throw new std::runtime_error("There can't be two prefixes in attribute name.");
            }

            pNamePrefix = std::move(pNameLocalPart);
            pNameLocalPart.reset(new std::string());
        }
        else if (std::isspace(cByte, m_aLocale) != 0)
        {
            cByte = ConsumeWhitespace();

            if (cByte == '\0')
            {
                throw new std::runtime_error("Attribute incomplete.");
            }
            else if (cByte != '=')
            {
                throw new std::runtime_error("Attribute name is malformed.");
            }

            // To make sure that the next loop iteration will end up in cByte == '='.
            m_aStream.unget();

            if (m_aStream.bad() == true)
            {
                throw new std::runtime_error("Stream is bad.");
            }
        }
        else if (cByte == '=')
        {
            if (pNamePrefix == nullptr)
            {
                pNamePrefix.reset(new std::string);
            }

            pName = std::unique_ptr<QName>(new QName("", *pNameLocalPart, *pNamePrefix));

            return true;
        }
        else if (std::isalnum(cByte, m_aLocale) == true ||
                 cByte == '-' ||
                 cByte == '_' ||
                 cByte == '.')
        {
            pNameLocalPart->push_back(cByte);
        }
        else
        {
            int nByte(cByte);
            std::stringstream strMessage;
            strMessage << "Character '" << cByte << "' (0x"
                       << std::hex << std::uppercase << nByte << std::nouppercase << std::dec
                       << ") not supported in an attribute name.";
            throw new std::runtime_error(strMessage.str());
        }

    } while (true);

    return false;
}

bool XMLEventReader::HandleAttributeValue(std::unique_ptr<std::string>& pValue)
{
    pValue = std::unique_ptr<std::string>(new std::string);
    char cDelimiter(ConsumeWhitespace());

    if (cDelimiter == '\0')
    {
        throw new std::runtime_error("Attribute is missing its value.");
    }
    else if (cDelimiter != '\'' &&
             cDelimiter != '"')
    {
        int nByte(cDelimiter);
        std::stringstream strMessage;
        strMessage << "Attribute value doesn't start with a delimiter like ''' or '\"', instead, '" << cDelimiter << "' (0x"
                    << std::hex << std::uppercase << nByte << std::nouppercase << std::dec
                    << ") was found.";
        throw new std::runtime_error(strMessage.str());
    }

    char cByte('\0');

    do
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            throw new std::runtime_error("Attribute value incomplete.");
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (cByte == cDelimiter)
        {
            return true;
        }
        else if (cByte == '&')
        {
            std::unique_ptr<std::string> pResolvedText = nullptr;

            ResolveEntity(pResolvedText);
            pValue->append(*pResolvedText);
        }
        else
        {
            pValue->push_back(cByte);
        }

    } while (true);

    return false;
}

void XMLEventReader::ResolveEntity(std::unique_ptr<std::string>& pResolvedText)
{
    if (pResolvedText != nullptr)
    {
        throw new std::invalid_argument("No nullptr passed.");
    }

    char cByte = '\0';
    m_aStream.get(cByte);

    if (m_aStream.eof() == true)
    {
        throw new std::runtime_error("Entity incomplete.");
    }

    if (m_aStream.bad() == true)
    {
        throw new std::runtime_error("Stream is bad.");
    }

    if (cByte == ';')
    {
        throw new std::runtime_error("Entity has no name.");
    }
    else
    {
        std::unique_ptr<std::string> pEntityName(new std::string);
        pEntityName->push_back(cByte);

        do
        {
            m_aStream.get(cByte);

            if (m_aStream.eof() == true)
            {
                throw new std::runtime_error("Entity incomplete.");
            }

            if (m_aStream.bad() == true)
            {
                throw new std::runtime_error("Stream is bad.");
            }

            if (cByte == ';')
            {
                break;
            }

            pEntityName->push_back(cByte);

        } while (true);

        std::map<std::string, std::string>::iterator iter = m_aEntityReplacementDictionary.find(*pEntityName);

        if (iter != m_aEntityReplacementDictionary.end())
        {
            pResolvedText = std::unique_ptr<std::string>(new std::string(iter->second));
        }
        else
        {
            std::stringstream aMessage;
            aMessage << "Unable to resolve entity '&" << *pEntityName << ";'.";
            throw new std::runtime_error(aMessage.str());
        }
    }
}

/**
 * @retval Returns the first non-whitespace character or '\0' in
 *     case of end-of-file.
 */
char XMLEventReader::ConsumeWhitespace()
{
    char cByte('\0');

    do
    {
        m_aStream.get(cByte);

        if (m_aStream.eof() == true)
        {
            return '\0';
        }

        if (m_aStream.bad() == true)
        {
            throw new std::runtime_error("Stream is bad.");
        }

        if (std::isspace(cByte, m_aLocale) == 0)
        {
            return cByte;
        }

    } while (true);
}

}
