#!/usr/bin/env python

# -*- mode: Python -*-
#
#       File:         setup.py
#       Date:         Thu Sep 26 16:31:07 2013
#
#       GNU recutils - Setup file for Python bindings

# Copyright (C) 2013 Maninya M.

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

from distutils.core import setup, Extension
setup(
    name = 'recutils', 
    version = '1.5',
    py_modules=['pyrec'],
    ext_modules = [
        Extension('recutils', ['recutils.c'],
                  libraries = [ 'rec' ],
                  ),
      ],
) 
