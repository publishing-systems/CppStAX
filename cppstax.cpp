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
 * @file $/cppstax.cpp
 * @brief Demo program.
 * @author Stephan Kreutzer
 * @since 2017-08-28
 */

#include "XMLInputFactory.h"
#include <memory>
#include <iostream>
#include <fstream>

typedef std::unique_ptr<cppstax::XMLEventReader> XMLEventReader;
typedef std::unique_ptr<cppstax::XMLEvent> XMLEvent;

int Run(std::istream& aStream);



int main(int argc, char* argv[])
{
    std::cout << "CppStAX Copyright (C) 2017-2018 Stephan Kreutzer\n"
              << "This program comes with ABSOLUTELY NO WARRANTY.\n"
              << "This is free software, and you are welcome to redistribute it\n"
              << "under certain conditions. See the GNU Affero General Public License 3\n"
              << "or any later version for details. Also, see the source code repository\n"
              << "https://github.com/publishing-systems/CppStAX/ and\n"
              << "the project website http://www.publishing-systems.org.\n"
              << std::endl;

    std::unique_ptr<std::ifstream> pStream = nullptr;

    try
    {
        if (argc >= 2)
        {
            pStream = std::unique_ptr<std::ifstream>(new std::ifstream);
            pStream->open(argv[1]);

            if (pStream->is_open() != true)
            {
                std::cout << "Couldn't open input file '" << argv[1] << "'.";
                return -1;
            }

            Run(*pStream);

            pStream->close();
        }
        else
        {
            std::cout << "Please enter your XML input data, confirm with Ctrl+D:" << std::endl;
            Run(std::cin);
        }
    }
    catch (std::exception* pException)
    {
        std::cout << "Exception: " << pException->what() << std::endl;

        if (pStream != nullptr)
        {
            if (pStream->is_open() == true)
            {
                pStream->close();
            }
        }

        return -1;
    }

    return 0;
}

int Run(std::istream& aStream)
{
    cppstax::XMLInputFactory aFactory;
    XMLEventReader pReader = aFactory.createXMLEventReader(aStream);

    // Instead of looking at XMLEvents sequentially, one could
    // also implement a "parse tree" to react to XMLEvents, so
    // writing state machines can be avoided because of the
    // implicit call tree context, pretty much like CppStAX
    // does it's parsing.
    while (pReader->hasNext() == true)
    {
        XMLEvent pEvent = pReader->nextEvent();

        if (pEvent->isStartElement() == true)
        {
            cppstax::StartElement& aStartElement = pEvent->asStartElement();
            const cppstax::QName& aName = aStartElement.getName();

            std::string strTag("<");
            std::string strPrefix(aName.getPrefix());

            if (!strPrefix.empty())
            {
                strTag += strPrefix;
                strTag += ":";
            }

            strTag += aName.getLocalPart();

            for (std::list<std::shared_ptr<cppstax::Attribute>>::iterator iter = aStartElement.getAttributes()->begin();
                 iter != aStartElement.getAttributes()->end();
                 iter++)
            {
                const cppstax::QName& aAttributeName = (*iter)->getName();

                strTag += " ";

                std::string strAttributePrefix(aAttributeName.getPrefix());

                if (!strAttributePrefix.empty())
                {
                    strTag += strAttributePrefix;
                    strTag += ":";
                }

                strTag += aAttributeName.getLocalPart();
                strTag += "=\"";

                const std::string& strCharacters((*iter)->getValue());

                for (std::string::const_iterator iter = strCharacters.begin();
                    iter != strCharacters.end();
                    iter++)
                {
                    switch (*iter)
                    {
                    case '\"':
                        strTag += "&quot;";
                        break;
                    default:
                        strTag += *iter;
                        break;
                    }
                }

                strTag += "\"";
            }

            strTag += ">";

            std::cout << strTag;
        }
        else if (pEvent->isEndElement() == true)
        {
            cppstax::EndElement& aEndElement = pEvent->asEndElement();
            cppstax::QName aName = aEndElement.getName();

            std::string strTag("</");
            std::string strPrefix(aName.getPrefix());

            if (!strPrefix.empty())
            {
                strTag += strPrefix;
                strTag += ":";
            }

            strTag += aName.getLocalPart();
            strTag += ">";

            std::cout << strTag;
        }
        else if (pEvent->isCharacters() == true)
        {
            cppstax::Characters& aCharacters = pEvent->asCharacters();
            const std::string& strCharacters(aCharacters.getData());

            for (std::string::const_iterator iter = strCharacters.begin();
                 iter != strCharacters.end();
                 iter++)
            {
                switch (*iter)
                {
                case '&':
                    std::cout << "&amp;";
                    break;
                case '<':
                    std::cout << "&lt;";
                    break;
                case '>':
                    std::cout << "&gt;";
                    break;
                default:
                    std::cout << *iter;
                    break;
                }
            }
        }
        else if (pEvent->isComment() == true)
        {
            std::cout << "<!--" << pEvent->asComment().getText() << "-->";
        }
        else if (pEvent->isProcessingInstruction() == true)
        {
            const cppstax::ProcessingInstruction& aPI = pEvent->asProcessingInstruction();

            std::cout << "<?" << aPI.getTarget() << " " << aPI.getData() << "?>";
        }
        else
        {

        }
    }

    return 0;
}
