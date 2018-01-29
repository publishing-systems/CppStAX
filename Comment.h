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
 * @file $/Comment.h
 * @author Stephan Kreutzer
 * @since 2017-09-03
 */

#ifndef _CPPSTAX_COMMENT_H
#define _CPPSTAX_COMMENT_H

#include <memory>
#include <string>

namespace cppstax
{

class Comment
{
public:
    Comment(std::unique_ptr<std::string> pText);

public:
    const std::string& getText() const;

protected:
    std::unique_ptr<std::string> m_pText;

};

}

#endif
