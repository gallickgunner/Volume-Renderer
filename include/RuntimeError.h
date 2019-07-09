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

#ifndef RUNTIMEERROR_H
#define RUNTIMEERROR_H

#include <string>
#include <exception>

class RuntimeError : public std::exception
{
    public:
        RuntimeError();
        RuntimeError(std::string msg);
        RuntimeError(std::string msg, std::string filename, std::string line_no);
        ~RuntimeError();

        virtual const char* what() const noexcept;
        const char* getFileName() const noexcept;
        const char* getLineNo() const noexcept;

    private:
        std::string msg;
        std::string filename;
        std::string line_no;
};

#endif // RUNTIMEERROR_H
