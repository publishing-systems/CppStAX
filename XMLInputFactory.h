/* Copyright (C) 2017 Stephan Kreutzer
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
 * @file $/XMLInputFactory.h
 * @author Stephan Kreutzer
 * @since 2017-08-24
 */


#ifndef _CPPSTAX_XMLINPUTFACTORY_H
#define _CPPSTAX_XMLINPUTFACTORY_H

#include "XMLEventReader.h"
#include <istream>
#include <memory>

namespace cppstax
{

class XMLInputFactory
{
public:
    std::unique_ptr<XMLEventReader> createXMLEventReader(std::istream& stream);

};

}

#endif
