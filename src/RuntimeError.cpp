/******************************************************************************
 *  Copyright (C) 2018 by Umair Ahmed.
 *
 *  This is a free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This software is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 *
 ******************************************************************************/

#include "RuntimeError.h"
#include <iostream>

RuntimeError::RuntimeError() : std::exception()
{
    //ctor
}

RuntimeError::RuntimeError(std::string msg) : std::exception(), msg(msg), filename(""), line_no("")
{

}

RuntimeError::RuntimeError(std::string msg, std::string filename, std::string line_no) : std::exception(),  msg(msg),
                                                                                                            filename(filename),
                                                                                                            line_no(line_no)
{
    //ctor
    if(!filename.empty())
        this->msg += (" in File: \"" + filename);
    if(!line_no.empty())
        this->msg += ("\" at Line number: " + line_no);

}

RuntimeError::~RuntimeError()
{
    //dtor
}

const char* RuntimeError::what() const noexcept
{
    return this->msg.c_str();
}

const char* RuntimeError::getFileName() const noexcept
{
    return filename.c_str();
}

const char* RuntimeError::getLineNo() const noexcept
{
    return line_no.c_str();
}

