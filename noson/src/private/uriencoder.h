/*
 *      Copyright (C) 2026 Jean-Luc Barriere
 *
 *  This library is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU Lesser General Public License as published
 *  by the Free Software Foundation; either version 3, or (at your option)
 *  any later version.
 *
 *  This library is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU Lesser General Public License for more details.
 *
 *  You should have received a copy of the GNU Lesser General Public License
 *  along with this library; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 51 Franklin Street, Fifth Floor, Boston,
 *  MA 02110-1301 USA
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#ifndef URIENCODER_H
#define URIENCODER_H

#include <string>

#define pathencode __pathencode
std::string pathencode(const std::string& str);

#define pathdecode __pathdecode
std::string pathdecode(const std::string& str);

#define urlencode __urlencode
std::string urlencode(const std::string& str);

#define urldecode __urldecode
std::string urldecode(const std::string& str);

#endif /* URIENCODER_H */
